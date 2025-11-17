/*  Copyright (C) 2024 Praxiom Health

    This file is part of Praxiom.
*/

#pragma once

#include <cstdint>

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
      
      int OnCommand(struct ble_gatt_access_ctxt* ctxt);
      
      // ✅ DIAGNOSTIC: Made inline to ensure it always returns current value
      uint32_t GetBasePraxiomAge() const {
        return basePraxiomAge;
      }

    private:
      // BLE Service Definition
      static constexpr uint16_t praxiomServiceId {0x1900};
      static constexpr uint16_t praxiomBioAgeCharId {0x1901};

      // Service UUID: 00001900-78fc-48fe-8e23-433b3a1942d0
      static constexpr ble_uuid128_t praxiomServiceUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e, 0xfe, 0x48, 0xfc, 0x78, 0x00, 0x00, 0x19, 0x00}
      };

      // Bio-Age Characteristic UUID: 00001901-78fc-48fe-8e23-433b3a1942d0
      static constexpr ble_uuid128_t praxiomBioAgeCharUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e, 0xfe, 0x48, 0xfc, 0x78, 0x01, 0x00, 0x19, 0x00}
      };

      struct ble_gatt_chr_def characteristicDefinition[2];
      struct ble_gatt_svc_def serviceDefinition[2];

      // Storage for Bio-Age value (sent from mobile app)
      uint32_t basePraxiomAge {0};
    };
  }
}
