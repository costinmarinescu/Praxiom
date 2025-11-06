/*  Copyright (C) 2021 Costin Marinescu
    
    This file is part of Praxiom.

    Praxiom is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Praxiom is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once

#include <cstdint>
#include <array>
#include <host/ble_gatt.h>
#include <host/ble_uuid.h>

namespace Pinetime {
  namespace Controllers {
    class PraxiomService {
    public:
      PraxiomService();
      void Init();
      
      // Get the current base Praxiom Age (set by phone app)
      uint32_t GetBasePraxiomAge() const {
        return basePraxiomAge;
      }
      
      // Set base age (called from WatchFace after sensor adjustments)
      void SetBasePraxiomAge(uint32_t age) {
        basePraxiomAge = age;
      }
      
      // Notify connected phone of current age
      void NotifyPraxiomAge(uint32_t age);

    private:
      // Static callback wrapper - CRITICAL for NimBLE
      static int OnPraxiomWriteCallback(uint16_t conn_handle,
                                        uint16_t attr_handle,
                                        struct ble_gatt_access_ctxt* ctxt,
                                        void* arg);

      // Member callback handler
      int OnPraxiomWrite(uint16_t conn_handle,
                        uint16_t attr_handle,
                        struct ble_gatt_access_ctxt* ctxt);

      // UUID definitions (custom 128-bit UUIDs)
      // Service UUID: 19000000-78fc-48fe-8e23-433b3a1942d0
      static constexpr ble_uuid128_t serviceUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
                  0xfe, 0x48, 0xfc, 0x78, 0x00, 0x00, 0x00, 0x19}
      };
      
      // Write characteristic: 19000001-78fc-48fe-8e23-433b3a1942d0
      static constexpr ble_uuid128_t writeCharUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
                  0xfe, 0x48, 0xfc, 0x78, 0x01, 0x00, 0x00, 0x19}
      };
      
      // Notify characteristic: 19000002-78fc-48fe-8e23-433b3a1942d0
      static constexpr ble_uuid128_t notifyCharUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
                  0xfe, 0x48, 0xfc, 0x78, 0x02, 0x00, 0x00, 0x19}
      };

      // Service structures - MUST have null terminators
      ble_gatt_chr_def characteristicDefinition[3]; // +1 for {0}
      ble_gatt_svc_def serviceDefinition[2];        // +1 for {0}

      uint16_t notifyCharHandle;
      uint32_t basePraxiomAge {0};
    };
  }
}
