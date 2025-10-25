#pragma once

#include <cstdint>
#include "host/ble_gap.h"

#define BLE_UUID_PRAXIOM_SERVICE 0x01, 0xb5, 0xa3, 0xf3, 0x93, 0xe0, 0xa9, 0xe5, 0x0e, 0x24, 0xdc, 0xca, 0x9e, 0x00, 0x40, 0x6e

namespace Pinetime {
  namespace Controllers {
    class NimbleController;
    
    class PraxiomHealthService {
    public:
      PraxiomHealthService(NimbleController& nimble);
      void Init();
      
      // Getters for watch to read data received from app
      uint16_t GetBioAge() const { return bioAge; }  // Returns age * 10
      uint8_t GetOverallHealthScore() const { return overallHealthScore; }
      uint8_t GetSpecificHealthScore() const { return specificHealthScore; }
      uint8_t GetFitnessLevel() const { return fitnessLevel; }
      
      // Setters for watch to send sensor data to app (future use)
      void SetHeartRate(uint8_t hr) { heartRate = hr; }
      void SetStepCount(uint32_t steps) { stepCount = steps; }
      void SetBatteryLevel(uint8_t battery) { batteryLevel = battery; }
      
    private:
      NimbleController& nimble;
      
      // Data received from app
      uint16_t bioAge = 0;           // Bio-Age × 10 (e.g., 385 = 38.5 years)
      uint8_t overallHealthScore = 0;
      uint8_t specificHealthScore = 0;
      uint8_t fitnessLevel = 0;
      
      // Sensor data from watch (for future app reading)
      uint8_t heartRate = 0;
      uint32_t stepCount = 0;
      uint8_t batteryLevel = 0;
      
      // BLE characteristic handles
      uint16_t bioAgeCharHandle;
      uint16_t healthDataCharHandle;
      uint16_t sensorDataCharHandle;
      
      // BLE characteristic UUIDs
      static constexpr ble_uuid128_t bioAgeCharUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0x02, 0xb5, 0xa3, 0xf3, 0x93, 0xe0, 0xa9, 0xe5, 0x0e, 0x24, 0xdc, 0xca, 0x9e, 0x00, 0x40, 0x6e}
      };
      
      static constexpr ble_uuid128_t healthDataCharUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0x03, 0xb5, 0xa3, 0xf3, 0x93, 0xe0, 0xa9, 0xe5, 0x0e, 0x24, 0xdc, 0xca, 0x9e, 0x00, 0x40, 0x6e}
      };
      
      static constexpr ble_uuid128_t sensorDataCharUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0x04, 0xb5, 0xa3, 0xf3, 0x93, 0xe0, 0xa9, 0xe5, 0x0e, 0x24, 0xdc, 0xca, 0x9e, 0x00, 0x40, 0x6e}
      };
      
      struct ble_gatt_chr_def characteristicDefinition[4];
      struct ble_gatt_svc_def serviceDefinition[2];
      
      // BLE callbacks
      static int OnBioAgeWrite(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt, void* arg);
      static int OnHealthDataWrite(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt, void* arg);
      static int OnSensorDataRead(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt, void* arg);
      
      // Helper functions
      static uint16_t ExtractUint16LE(const uint8_t* data);
      static uint32_t ExtractUint32LE(const uint8_t* data);
    };
  }
}
