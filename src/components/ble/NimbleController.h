#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#define min
#define max
#include <host/ble_gap.h>
#undef max
#undef min

#include "AlertNotificationClient.h"
#include "AlertNotificationService.h"
#include "BatteryInformationService.h"
#include "BleController.h"
#include "CurrentTimeClient.h"
#include "CurrentTimeService.h"
#include "DeviceInformationService.h"
#include "DfuService.h"
#include "HeartRateService.h"
#include "ImmediateAlertService.h"
#include "MusicService.h"
#include "NavigationService.h"
#include "ServiceDiscovery.h"
#include "MotionService.h"
#include "weather/WeatherService.h"
#include "PraxiomService.h"

namespace Pinetime {
  namespace Drivers {
    class SpiNorFlash;
  }
  namespace Drivers {
    class I2C;
  }
  namespace Controllers {
    class Ble;
    class DateTime;
    class NotificationManager;
    class Battery;
    class HeartRateController;
    class MotionController;
    class FS;
    class PraxiomController;

    class NimbleController {
    public:
      NimbleController(Pinetime::System::SystemTask& systemTask,
                       Ble& bleController,
                       DateTime& dateTimeController,
                       NotificationManager& notificationManager,
                       Battery& batteryController,
                       Pinetime::Drivers::SpiNorFlash& spiNorFlash,
                       HeartRateController& heartRateController,
                       MotionController& motionController,
                       Pinetime::Controllers::FS& fs,
                       PraxiomController& praxiomController);

      void Init();
      void StartAdvertising();
      int OnGAPEvent(ble_gap_event* event);

      int OnDiscoveryEvent(uint16_t i, const ble_gatt_error* pError, const ble_gatt_svc* pSvc);
      int OnCTSCharacteristicDiscoveryEvent(uint16_t connectionHandle, const ble_gatt_error* error, const ble_gatt_chr* characteristic);
      int OnANSCharacteristicDiscoveryEvent(uint16_t connectionHandle, const ble_gatt_error* error, const ble_gatt_chr* characteristic);
      int OnCurrentTimeReadResult(uint16_t connectionHandle, const ble_gatt_error* error, ble_gatt_attr* attribute);
      int OnANSDescriptorDiscoveryEventCallback(uint16_t connectionHandle,
                                                const ble_gatt_error* error,
                                                uint16_t characteristicValueHandle,
                                                const ble_gatt_dsc* descriptor);

      void StartDiscovery();

      Pinetime::Controllers::MusicService& music() {
        return musicService;
      }
      Pinetime::Controllers::NavigationService& navigation() {
        return navService;
      }
      Pinetime::Controllers::AlertNotificationService& alertService() {
        return anService;
      }
      Pinetime::Controllers::WeatherService& weather() {
        return weatherService;
      }

      uint16_t connHandle();
      void NotifyBatteryLevel(uint8_t level);

      void RestartFastAdv() {
        fastAdvCount = 0;
      }

      void EnableRadio();
      void DisableRadio();

    private:
      void PersistBond(struct ble_gap_conn_desc& desc);
      void RestoreBond();

      static constexpr const char* deviceName = "InfiniTime";
      Pinetime::System::SystemTask& systemTask;
      Ble& bleController;
      DateTime& dateTimeController;
      NotificationManager& notificationManager;
      Pinetime::Drivers::SpiNorFlash& spiNorFlash;
      Pinetime::Controllers::FS& fs;
      Pinetime::Controllers::DfuService dfuService;

      DeviceInformationService deviceInformationService;
      CurrentTimeClient currentTimeClient;
      AlertNotificationService anService;
      AlertNotificationClient alertNotificationClient;
      CurrentTimeService currentTimeService;
      MusicService musicService;
      WeatherService weatherService;
      NavigationService navService;
      BatteryInformationService batteryInformationService;
      ImmediateAlertService immediateAlertService;
      HeartRateService heartRateService;
      MotionService motionService;
      PraxiomService praxiomService;

      uint8_t addrType;
      uint16_t connectionHandle = BLE_HS_CONN_HANDLE_NONE;
      uint8_t fastAdvCount = 0;
      uint8_t bondId[16] = {0};

      ble_uuid128_t dfuServiceUuid {
        .u {.type = BLE_UUID_TYPE_128},
        .value = {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x30, 0x15, 0x00, 0x00}};

      ServiceDiscovery serviceDiscovery;
    };

    static NimbleController* nptr;
  }
}
