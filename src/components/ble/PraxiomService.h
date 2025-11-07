/*  Copyright (C) 2024 Praxiom Health

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

#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_gap.h>
#include <host/ble_uuid.h>
#undef max
#undef min

int PraxiomCallback(uint16_t connHandle, uint16_t attrHandle, struct ble_gatt_access_ctxt* ctxt, void* arg);

namespace Pinetime {
  namespace Controllers {

    class PraxiomService {
    public:
      PraxiomService();

      void Init();

      int OnCommand(struct ble_gatt_access_ctxt* ctxt);

      // Get base Praxiom Age (set by mobile app via BLE)
      uint32_t GetBasePraxiomAge() const { return basePraxiomAge; }

    private:
      // Service UUID: 00001900-78fc-48fe-8e23-433b3a1942d0
      static constexpr ble_uuid128_t BaseUuid() {
        return CharUuid(0x00, 0x00);
      }

      // 000019yyxx-78fc-48fe-8e23-433b3a1942d0
      static constexpr ble_uuid128_t CharUuid(uint8_t x, uint8_t y) {
        return ble_uuid128_t {.u = {.type = BLE_UUID_TYPE_128},
                              .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e, 0xfe, 0x48, 0xfc, 0x78, y, x, 0x19, 0x00}};
      }

      ble_uuid128_t praxiomServiceUuid {BaseUuid()};

      // Write characteristic UUID: 00001901-78fc-48fe-8e23-433b3a1942d0
      ble_uuid128_t praxiomAgeCharUuid {CharUuid(0x01, 0x00)};

      const struct ble_gatt_chr_def characteristicDefinition[2] = {{.uuid = &praxiomAgeCharUuid.u,
                                                                    .access_cb = PraxiomCallback,
                                                                    .arg = this,
                                                                    .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ,
                                                                    .val_handle = &eventHandle},
                                                                   {0}};
      const struct ble_gatt_svc_def serviceDefinition[2] = {
        {.type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = &praxiomServiceUuid.u, .characteristics = characteristicDefinition},
        {0}};

      uint16_t eventHandle {};

      uint32_t basePraxiomAge = 53; // Default demo value
    };
  }
}
