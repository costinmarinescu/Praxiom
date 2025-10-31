#pragma once

#include <cstdint>

#define min
#define max
#include <host/ble_gap.h>
#include <host/ble_uuid.h>
#undef max
#undef min

#include <timers.h>

#include "components/settings/Settings.h"
#include "components/datetime/DateTimeController.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"

struct ble_gatt_access_ctxt;

namespace Pinetime {
  namespace Controllers {
    class NimbleController;

    class PraxiomService {
    public:
      PraxiomService(NimbleController& nimble,
                     DateTime& dateTimeController,
                     HeartRateController& heartRateController,
                     MotionController& motionController);

      static void BindSettings(Settings& settingsController);

      void Init();
      int OnCommand(uint16_t attributeHandle, struct ble_gatt_access_ctxt* ctxt);

      void SubscribeNotification(uint16_t attributeHandle);
      void UnsubscribeNotification(uint16_t attributeHandle);

      void NotifyCurrentHealthData();
      void OnConnected();
      void OnDisconnected();

      static constexpr ble_uuid128_t serviceUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e}};

    private:
      void HandleBioAgeWrite(uint16_t ageTenths);
      void HandlePackageWrite(const uint8_t* data, uint16_t length);
      void UpdateLastSync();

      NimbleController& nimble;
      DateTime& dateTimeController;
      HeartRateController& heartRateController;
      MotionController& motionController;
      static Settings* settings;

      static constexpr ble_uuid128_t bioAgeUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x02, 0x00, 0x40, 0x6e}};

      static constexpr ble_uuid128_t packageUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x03, 0x00, 0x40, 0x6e}};

      const struct ble_gatt_chr_def characteristicDefinition[3];
      const struct ble_gatt_svc_def serviceDefinition[2];

      uint16_t bioAgeHandle = 0;
      uint16_t packageHandle = 0;
      bool packageNotifyEnabled = false;
      TimerHandle_t periodicTimer {};
    };
  }
}
