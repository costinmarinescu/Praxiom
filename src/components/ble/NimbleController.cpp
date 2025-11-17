#include "components/ble/NimbleController.h"
#include <libraries/log/nrf_log.h>
#include "systemtask/SystemTask.h"

using namespace Pinetime::Controllers;

NimbleController* Pinetime::Controllers::nptr;

NimbleController::NimbleController(Pinetime::System::SystemTask& systemTask,
                                   Pinetime::Controllers::Ble& bleController,
                                   DateTime& dateTimeController,
                                   NotificationManager& notificationManager,
                                   Controllers::Battery& batteryController,
                                   Pinetime::Drivers::SpiNorFlash& spiNorFlash,
                                   Controllers::HeartRateController& heartRateController,
                                   Controllers::MotionController& motionController,
                                   Controllers::FS& fs)
  : systemTask {systemTask},
    bleController {bleController},
    dateTimeController {dateTimeController},
    notificationManager {notificationManager},
    spiNorFlash {spiNorFlash},
    fs {fs},
    dfuService {spiNorFlash, bleController},
    fsService {fs},
    deviceInformationService {},
    currentTimeClient {dateTimeController},
    anService {systemTask, notificationManager},
    alertNotificationClient {systemTask, notificationManager},
    currentTimeService {dateTimeController},
    musicService {systemTask},
    weatherService {systemTask, dateTimeController},
    navigationService {systemTask},
    batteryInformationService {batteryController},
    immediateAlertService {systemTask, notificationManager},
    heartRateService {systemTask, heartRateController},
    motionService {systemTask, motionController},      // ✅ Motion service
    praxiomService {},                                  // ✅ Praxiom service
    serviceDiscovery({&currentTimeClient, &alertNotificationClient}) {
  nptr = this;
}

void NimbleController::Init() {
  // Initialize NimBLE stack
  ble_hs_cfg.reset_cb = OnResetCallback;
  ble_hs_cfg.sync_cb = OnSyncCallback;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

  // Initialize all services
  dfuService.Init();
  fsService.Init();
  deviceInformationService.Init();
  currentTimeService.Init();
  anService.Init();
  musicService.Init();
  weatherService.Init();
  navigationService.Init();
  batteryInformationService.Init();
  immediateAlertService.Init();
  heartRateService.Init();
  motionService.Init();          // ✅ Initialize motion service
  praxiomService.Init();         // ✅ Initialize Praxiom service
  
  NRF_LOG_INFO("✅ All BLE services initialized, including PraxiomService");

  // Start NimBLE host task
  nimble_port_freertos_init(NimbleController::HostTask);
}

void NimbleController::StartAdvertising() {
  // Setup advertising data
  ble_gap_adv_params adv_params;
  memset(&adv_params, 0, sizeof(adv_params));
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

  // Set device name
  ble_svc_gap_device_name_set("InfiniTime");

  // Start advertising
  int rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, 
                             NimbleController::OnGAPEvent, this);
  
  if (rc == 0) {
    NRF_LOG_INFO("✅ BLE advertising started");
  } else {
    NRF_LOG_ERROR("❌ Failed to start advertising: %d", rc);
  }
}

int NimbleController::OnGAPEvent(ble_gap_event* event) {
  switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
      NRF_LOG_INFO("✅ BLE device connected");
      connectionHandle = event->connect.conn_handle;
      bleController.Connect();
      systemTask.PushMessage(Pinetime::System::Messages::BleConnected);
      
      // ✅ START: Enable continuous sensor broadcasting on connection
      EnableContinuousSensorBroadcasting();
      break;

    case BLE_GAP_EVENT_DISCONNECT:
      NRF_LOG_INFO("⚠️ BLE device disconnected");
      connectionHandle = BLE_HS_CONN_HANDLE_NONE;
      bleController.Disconnect();
      systemTask.PushMessage(Pinetime::System::Messages::BleDisconnected);
      
      // Restart advertising
      StartAdvertising();
      
      // ✅ STOP: Disable continuous broadcasting to save battery
      DisableContinuousSensorBroadcasting();
      break;

    case BLE_GAP_EVENT_CONN_UPDATE:
      NRF_LOG_INFO("Connection parameters updated");
      break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
      NRF_LOG_INFO("Advertising complete, restarting");
      StartAdvertising();
      break;

    case BLE_GAP_EVENT_MTU:
      NRF_LOG_INFO("MTU updated: %d", event->mtu.value);
      break;

    default:
      break;
  }
  return 0;
}

// ✅ NEW: Enable continuous sensor data broadcasting
void NimbleController::EnableContinuousSensorBroadcasting() {
  NRF_LOG_INFO("🔄 Enabling continuous sensor broadcasting");
  
  // Enable heart rate continuous mode
  heartRateService.EnableContinuousMode();
  
  // Motion service will broadcast steps when screen is on
  // (InfiniTime limitation - steps only sent when display active)
  motionService.EnablePeriodicUpdates();
  
  // Battery service already sends on changes
  NRF_LOG_INFO("✅ Continuous sensor broadcasting enabled");
}

// ✅ NEW: Disable continuous sensor broadcasting
void NimbleController::DisableContinuousSensorBroadcasting() {
  NRF_LOG_INFO("🔄 Disabling continuous sensor broadcasting");
  
  heartRateService.DisableContinuousMode();
  motionService.DisablePeriodicUpdates();
  
  NRF_LOG_INFO("✅ Continuous sensor broadcasting disabled");
}

void NimbleController::NotifyBatteryLevel(uint8_t level) {
  batteryInformationService.NotifyBatteryLevel(level);
}

uint16_t NimbleController::GetConnHandle() const {
  return connectionHandle;
}

void NimbleController::EnableRadio() {
  // Enable radio and start advertising
  StartAdvertising();
  bleController.EnableRadio();
}

void NimbleController::DisableRadio() {
  // Stop advertising and disable radio
  ble_gap_adv_stop();
  bleController.DisableRadio();
}

void NimbleController::RestartFastAdv() {
  fastAdvCount = 0;
  StartAdvertising();
}

void NimbleController::PersistBond(struct ble_gap_conn_desc& desc) {
  // Store bonding information
  memcpy(bondId, desc.peer_id_addr.val, 16);
}

void NimbleController::RestoreBond() {
  // Restore bonding information
  // Implementation depends on storage mechanism
}

void NimbleController::StartDiscovery() {
  serviceDiscovery.StartDiscovery(connectionHandle);
}

int NimbleController::OnDiscoveryEvent(uint16_t connectionHandle, const ble_gatt_error* error, const ble_gatt_svc* service) {
  return serviceDiscovery.OnDiscoveryEvent(connectionHandle, error, service);
}

int NimbleController::OnCTSCharacteristicDiscoveryEvent(uint16_t connectionHandle, 
                                                        const ble_gatt_error* error, 
                                                        const ble_gatt_chr* characteristic) {
  return currentTimeClient.OnCharacteristicDiscoveryEvent(connectionHandle, error, characteristic);
}

int NimbleController::OnANSCharacteristicDiscoveryEvent(uint16_t connectionHandle,
                                                        const ble_gatt_error* error,
                                                        const ble_gatt_chr* characteristic) {
  return alertNotificationClient.OnCharacteristicDiscoveryEvent(connectionHandle, error, characteristic);
}

int NimbleController::OnCurrentTimeReadResult(uint16_t connectionHandle,
                                              const ble_gatt_error* error,
                                              ble_gatt_attr* attribute) {
  return currentTimeClient.OnReadResult(connectionHandle, error, attribute);
}

int NimbleController::OnANSDescriptorDiscoveryEventCallback(uint16_t connectionHandle,
                                                            const ble_gatt_error* error,
                                                            uint16_t characteristicValueHandle,
                                                            const ble_gatt_dsc* descriptor) {
  return alertNotificationClient.OnDescriptorDiscoveryEvent(connectionHandle, error, 
                                                            characteristicValueHandle, descriptor);
}
