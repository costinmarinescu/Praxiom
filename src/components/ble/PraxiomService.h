#pragma once

#include <cstdint>
#include <array>

#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_gap.h>
#undef max
#undef min

namespace Pinetime {
  namespace Controllers {
    class PraxiomService {
    public:
      PraxiomService();
      void Init();
      
      int OnBioAgeWrite(uint16_t conn_handle, uint16_t attr_handle,
                        struct ble_gatt_access_ctxt* ctxt);
      
      int OnHealthMetricsRead(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt* ctxt);
      
      int OnStatusRead(uint16_t conn_handle, uint16_t attr_handle,
                       struct ble_gatt_access_ctxt* ctxt);
      
      // Getters for watchface
      float GetBioAge() const { return bioAge; }
      bool HasReceivedBioAge() const { return bioAgeReceived; }
      
      // Setters for health data (called by other services)
      void UpdateHeartRate(uint8_t hr);
      void UpdateSteps(uint32_t steps);
      void UpdateBattery(uint8_t batteryPercent);
      
      // Notification control
      void NotifyHealthMetrics();
      
    private:
      static constexpr uint16_t praxiomServiceUuid = 0x1900;
      static constexpr uint16_t bioAgeCharUuid = 0x1901;
      static constexpr uint16_t healthMetricsCharUuid = 0x1902;
      static constexpr uint16_t statusCharUuid = 0x1903;
      
      static constexpr ble_uuid128_t praxiomServiceBaseUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
                 0xfe, 0x48, 0xfc, 0x78, 0x00, 0x19, 0x00, 0x00}
      };
      
      static constexpr ble_uuid128_t bioAgeCharBaseUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
                 0xfe, 0x48, 0xfc, 0x78, 0x01, 0x19, 0x00, 0x00}
      };
      
      static constexpr ble_uuid128_t healthMetricsCharBaseUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
                 0xfe, 0x48, 0xfc, 0x78, 0x02, 0x19, 0x00, 0x00}
      };
      
      static constexpr ble_uuid128_t statusCharBaseUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
                 0xfe, 0x48, 0xfc, 0x78, 0x03, 0x19, 0x00, 0x00}
      };
      
      struct ble_gatt_chr_def characteristicDefinition[4];
      struct ble_gatt_svc_def serviceDefinition[2];
      
      uint16_t bioAgeHandle;
      uint16_t healthMetricsHandle;
      uint16_t statusHandle;
      
      // Data storage
      float bioAge;
      bool bioAgeReceived;
      uint8_t heartRate;
      uint32_t steps;
      uint8_t batteryPercent;
      uint8_t statusFlags;
    };
  }
}
