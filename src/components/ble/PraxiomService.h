#pragma once

#include <cstdint>

#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_gatt.h>
#include <host/ble_uuid.h>
#undef max
#undef min

// Forward declare the callback
int PraxiomCallback(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt, void* arg);

namespace Pinetime {
  namespace Controllers {
    class PraxiomService {
    public:
      PraxiomService();
      void Init();
      
      // Methods for managing Bio-Age
      void SetBasePraxiomAge(uint32_t age);
      uint32_t GetBasePraxiomAge() const;
      void NotifyPraxiomAge(uint32_t age);
      
      // Callback handler
      int OnCommand(struct ble_gatt_access_ctxt* ctxt);

    private:
      // UUID definitions - LITTLE-ENDIAN byte order
      // Base UUID: xxxxxxxx-78fc-48fe-8e23-433b3a1942d0
      
      // Service: 0x1900 -> 19000000-78fc-48fe-8e23-433b3a1942d0
      ble_uuid128_t serviceUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
                  0xfe, 0x48, 0xfc, 0x78, 0x00, 0x00, 0x00, 0x19}
      };

      // Write characteristic: 0x1901 -> 19000001-78fc-48fe-8e23-433b3a1942d0
      ble_uuid128_t writeCharUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
                  0xfe, 0x48, 0xfc, 0x78, 0x01, 0x00, 0x00, 0x19}
      };

      // Notify characteristic: 0x1902 -> 19000002-78fc-48fe-8e23-433b3a1942d0
      ble_uuid128_t notifyCharUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
                  0xfe, 0x48, 0xfc, 0x78, 0x02, 0x00, 0x00, 0x19}
      };

      // CRITICAL: Arrays MUST be const (like SimpleWeatherService)
      const struct ble_gatt_chr_def characteristicDefinition[3] = {
        {
          .uuid = &writeCharUuid.u,
          .access_cb = PraxiomCallback,
          .arg = this,
          .flags = BLE_GATT_CHR_F_WRITE
        },
        {
          .uuid = &notifyCharUuid.u,
          .access_cb = PraxiomCallback,
          .arg = this,
          .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
          .val_handle = &notifyCharHandle
        },
        {0}  // NULL TERMINATOR
      };
      
      const struct ble_gatt_svc_def serviceDefinition[2] = {
        {
          .type = BLE_GATT_SVC_TYPE_PRIMARY,
          .uuid = &serviceUuid.u,
          .characteristics = characteristicDefinition
        },
        {0}  // NULL TERMINATOR
      };

      uint16_t notifyCharHandle {};
      uint32_t basePraxiomAge {0};
    };
  }
}
