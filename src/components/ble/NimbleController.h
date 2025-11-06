#pragma once

#include <cstdint>

#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_gap.h>
#undef max
#undef min

#include "AlertNotificationClient.h"
#include "AlertNotificationService.h"
#include "BatteryInformationService.h"
#include "CurrentTimeClient.h"
#include "CurrentTimeService.h"
#include "DeviceInformationService.h"
#include "DfuService.h"
#include "FSService.h"
#include "HeartRateService.h"
#include "ImmediateAlertService.h"
#include "MotionService.h"
#include "MusicService.h"
#include "NavigationService.h"
#include "ServiceDiscovery.h"
#include "SimpleWeatherService.h"
#include "PraxiomService.h"  // ← ADDED

namespace Pinetime {
  namespace Drivers {
    class SpiNorFlash;
  }
  namespace Controllers {
    class Ble;
    class DateTime;
    class NotificationManager;
    class Battery;
    class HeartRateController;
    class MotionController;
    class FS;

    class NimbleController {
    public:
      NimbleController(Pinetime::System::SystemTask& systemTask,
                       Pinetime::Controllers::Ble& bleController,
                       DateTime& dateTimeController,
                       NotificationManager& notificationManager,
                       Controllers::Battery& batteryController,
                       Pinetime::Drivers::SpiNorFlash& spiNorFlash,
                       Controllers::HeartRateController& heartRateController,
                       Controllers::MotionController& motionController,
                       Controllers::FS& fs);

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

      Pinetime::Controllers::MusicService& GetMusicService() {
        return musicService;
      }
      Pinetime::Controllers::NavigationService& GetNavigationService() {
        return navigationService;
      }
      Pinetime::Controllers::AlertNotificationService& GetAlertNotificationService() {
        return anService;
      }
      Pinetime::Controllers::SimpleWeatherService& GetWeatherService() {
        return weatherService;
      }
      Pinetime::Controllers::PraxiomService& GetPraxiomService() {  // ← ADDED
        return praxiomService;
      }

      // ========== BACKWARD COMPATIBILITY WRAPPERS (START) ==========
      // These methods provide backward compatibility with old code that
      // uses the short method names instead of the Get* prefix
      
      Pinetime::Controllers::MusicService& music() {
        return GetMusicService();
      }
      
      Pinetime::Controllers::NavigationService& navigation() {
        return GetNavigationService();
      }
      
      Pinetime::Controllers::SimpleWeatherService& weather() {
        return GetWeatherService();
      }
      
      Pinetime::Controllers::AlertNotificationService& alertService() {
        return GetAlertNotificationService();
      }
      
      uint16_t connHandle() {
        return GetConnHandle();
      }
      // ========== BACKWARD COMPATIBILITY WRAPPERS (END) ==========

      uint16_t GetConnHandle() const;
      void NotifyBatteryLevel(uint8_t level);

      void RestartFastAdv() {
        fastAdvCount = 0;
      }

      void EnableRadio();
      void DisableRadio();

    private:
      void PersistBond(struct ble_gap_conn_desc& desc);
      void RestoreBond();

      static constexpr const char* deviceName = "Praxiom";  // Changed from InfiniTime
      Pinetime::System::SystemTask& systemTask;
      Pinetime::Controllers::Ble& bleController;
      DateTime& dateTimeController;
      NotificationManager& notificationManager;
      Pinetime::Drivers::SpiNorFlash& spiNorFlash;
      Controllers::FS& fs;
      Pinetime::Controllers::DfuService dfuService;
      Pinetime::Controllers::FSService fsService;

      DeviceInformationService deviceInformationService;
      CurrentTimeClient currentTimeClient;
      AlertNotificationService anService;
      AlertNotificationClient alertNotificationClient;
      CurrentTimeService currentTimeService;
      MusicService musicService;
      SimpleWeatherService weatherService;
      NavigationService navigationService;
      BatteryInformationService batteryInformationService;
      ImmediateAlertService immediateAlertService;
      HeartRateService heartRateService;
      MotionService motionService;  // ← ADDED
      PraxiomService praxiomService;  // ← ADDED
      ServiceDiscovery serviceDiscovery;

      uint8_t addrType;
      uint16_t connectionHandle = BLE_HS_CONN_HANDLE_NONE;
      uint8_t fastAdvCount = 0;
      uint8_t bondId[16] = {0};

      ble_uuid128_t dfuServiceUuid {
        .u {.type = BLE_UUID_TYPE_128},
        .value = {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x30, 0x15, 0x00, 0x00}};
    };

    static NimbleController* nptr;
  }
}
