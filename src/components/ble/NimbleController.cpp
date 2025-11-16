#include "components/ble/NimbleController.h"

#include <cstring>
#include <hal/nrf_rtc.h>
#define min
#define max
#include <host/ble_gap.h>
#include <host/ble_hs_adv.h>
#include <host/util/util.h>
#include <nimble/hci_common.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>
#undef max
#undef min
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/datetime/DateTimeController.h"
#include "systemtask/SystemTask.h"

using namespace Pinetime::Controllers;

NimbleController* Pinetime::Controllers::nptr;

NimbleController::NimbleController(Pinetime::System::SystemTask& systemTask,
                                   Ble& bleController,
                                   DateTime& dateTimeController,
                                   NotificationManager& notificationManager,
                                   Battery& batteryController,
                                   Pinetime::Drivers::SpiNorFlash& spiNorFlash,
                                   HeartRateController& heartRateController,
                                   MotionController& motionController,
                                   Pinetime::Controllers::FS& fs,
                                   PraxiomController& praxiomController)
  : systemTask {systemTask},
    bleController {bleController},
    dateTimeController {dateTimeController},
    notificationManager {notificationManager},
    spiNorFlash {spiNorFlash},
    fs {fs},
    dfuService {systemTask, bleController, spiNorFlash},
    currentTimeClient {dateTimeController},
    anService {systemTask, notificationManager},
    alertNotificationClient {systemTask, notificationManager},
    currentTimeService {dateTimeController},
    musicService {systemTask},
    weatherService {systemTask, dateTimeController},
    navService {systemTask},
    batteryInformationService {batteryController},
    immediateAlertService {systemTask, notificationManager},
    heartRateService {systemTask, heartRateController},
    motionService {systemTask, motionController},
    praxiomService {praxiomController},
    serviceDiscovery({&currentTimeClient, &alertNotificationClient}) {
  nptr = this;
}

void NimbleController::Init() {
  while (!ble_hs_synced()) {
  }

  nptr = this;
  ble_hs_cfg.store_status_cb = OnBondEvent;

  ble_svc_gap_init();
  ble_svc_gatt_init();

  deviceInformationService.Init();
  currentTimeClient.Init();
  currentTimeService.Init();
  musicService.Init();
  weatherService.Init();
  navService.Init();
  anService.Init();
  dfuService.Init();
  batteryInformationService.Init();
  immediateAlertService.Init();
  heartRateService.Init();
  motionService.Init();
  praxiomService.Init();

  int rc = ble_hs_util_ensure_addr(0);
  ASSERT(rc == 0);
  rc = ble_hs_id_infer_auto(0, &addrType);
  ASSERT(rc == 0);
  rc = ble_svc_gap_device_name_set(deviceName);
  ASSERT(rc == 0);
  rc = ble_svc_gap_device_appearance_set(0xC2);
  ASSERT(rc == 0);
  Pinetime::Controllers::Ble::BleAddress address = bleController.Address();
  rc = ble_hs_id_set_rnd(address.data());
  ASSERT(rc == 0);

  rc = ble_gatts_start();
  ASSERT(rc == 0);

  RestoreBond();
}

void NimbleController::StartAdvertising() {
  if (bleController.IsConnected() || ble_gap_adv_active() || ble_gap_disc_active()) {
    return;
  }

  ble_svc_gap_device_name_set(deviceName);

  ble_hs_adv_fields fields {};
  fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
  fields.uuids128 = &dfuServiceUuid;
  fields.num_uuids128 = 1;
  fields.uuids128_is_complete = 1;
  fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
  fields.tx_pwr_lvl_is_present = 1;

  int rc = ble_gap_adv_set_fields(&fields);
  ASSERT(rc == 0);

  ble_hs_adv_fields rsp_fields {};
  rsp_fields.name = reinterpret_cast<const uint8_t*>(deviceName);
  rsp_fields.name_len = strlen(deviceName);
  rsp_fields.name_is_complete = 1;

  rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
  ASSERT(rc == 0);

  ble_gap_adv_params adv_params {};
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

  if (fastAdvCount < 5) {
    adv_params.itvl_min = 32;
    adv_params.itvl_max = 47;
    fastAdvCount++;
  } else {
    adv_params.itvl_min = 1600;
    adv_params.itvl_max = 4000;
  }

  rc = ble_gap_adv_start(addrType, nullptr, 180000, &adv_params, OnGAPEvent, this);
  ASSERT(rc == 0);
}

int NimbleController::OnGAPEvent(ble_gap_event* event) {
  return nptr->OnGAPEvent(event);
}

int NimbleController::OnGAPEvent(ble_gap_event* event) {
  switch (event->type) {
    case BLE_GAP_EVENT_CONNECT: {
      if (event->connect.status != 0) {
        StartAdvertising();
      } else {
        connectionHandle = event->connect.conn_handle;
        bleController.Connect();
        systemTask.PushMessage(System::Messages::BleConnected);

        ble_gap_conn_desc desc;
        ble_gap_conn_find(event->connect.conn_handle, &desc);
        if (desc.sec_state.bonded) {
          PersistBond(desc);
        }
      }
    } break;

    case BLE_GAP_EVENT_DISCONNECT:
      connectionHandle = BLE_HS_CONN_HANDLE_NONE;
      bleController.Disconnect();
      fastAdvCount = 0;
      StartAdvertising();
      break;

    case BLE_GAP_EVENT_CONN_UPDATE:
      break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
      StartAdvertising();
      break;

    case BLE_GAP_EVENT_ENC_CHANGE: {
      ble_gap_conn_desc desc;
      ble_gap_conn_find(event->enc_change.conn_handle, &desc);
      if (desc.sec_state.bonded) {
        PersistBond(desc);
      }
      return 0;
    }

    case BLE_GAP_EVENT_REPEAT_PAIRING: {
      ble_gap_conn_desc desc;
      ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
      ble_store_util_delete_peer(&desc.peer_id_addr);
      return BLE_GAP_REPEAT_PAIRING_RETRY;
    }

    case BLE_GAP_EVENT_NOTIFY_TX:
      break;

    case BLE_GAP_EVENT_SUBSCRIBE:
      break;

    case BLE_GAP_EVENT_MTU:
      break;

    case BLE_GAP_EVENT_DISC_COMPLETE:
      serviceDiscovery.OnDiscoveryComplete();
      break;

    default:
      break;
  }
  return 0;
}

void NimbleController::StartDiscovery() {
  serviceDiscovery.StartDiscovery(connectionHandle);
}

uint16_t NimbleController::connHandle() {
  return connectionHandle;
}

void NimbleController::NotifyBatteryLevel(uint8_t level) {
  if (connectionHandle != BLE_HS_CONN_HANDLE_NONE) {
    batteryInformationService.NotifyBatteryLevel(connectionHandle, level);
  }
}

void NimbleController::PersistBond(struct ble_gap_conn_desc& desc) {
  union ble_store_key key;
  union ble_store_value value;

  memcpy(&key.sec, &desc.our_id_addr, sizeof(ble_addr_t));
  memcpy(&key.sec.peer_addr, &desc.peer_id_addr, sizeof(ble_addr_t));

  int rc = ble_store_read_peer_sec(&key.sec, &value.sec);
  if (rc == 0 && value.sec.ltk_present) {
    if (memcmp(bondId, value.sec.irk, 16) != 0) {
      memcpy(bondId, value.sec.irk, 16);
      fs.WriteBondId(bondId);
    }
  }
}

void NimbleController::RestoreBond() {
  fs.ReadBondId(bondId);
}

int NimbleController::OnBondEvent(struct ble_gap_event* event, void* arg) {
  return 0;
}

void NimbleController::EnableRadio() {
  NRF_RADIO->POWER = 1;
}

void NimbleController::DisableRadio() {
  NRF_RADIO->POWER = 0;
}

int NimbleController::OnDiscoveryEvent(uint16_t i, const ble_gatt_error* pError, const ble_gatt_svc* pSvc) {
  return serviceDiscovery.OnDiscoveryEvent(i, pError, pSvc);
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

int NimbleController::OnCurrentTimeReadResult(uint16_t connectionHandle, const ble_gatt_error* error, ble_gatt_attr* attribute) {
  return currentTimeClient.OnCurrentTimeReadResult(connectionHandle, error, attribute);
}

int NimbleController::OnANSDescriptorDiscoveryEventCallback(uint16_t connectionHandle,
                                                            const ble_gatt_error* error,
                                                            uint16_t characteristicValueHandle,
                                                            const ble_gatt_dsc* descriptor) {
  return alertNotificationClient.OnDescriptorDiscoveryEventCallback(connectionHandle, error, characteristicValueHandle, descriptor);
}
