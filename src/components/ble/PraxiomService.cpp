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

#include "components/ble/PraxiomService.h"

#include <cstring>
#include <nrf_log.h>

using namespace Pinetime::Controllers;

namespace {
  uint32_t ToUInt32(const uint8_t* data) {
    return data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
  }
}

int PraxiomCallback(uint16_t /*connHandle*/, uint16_t /*attrHandle*/, struct ble_gatt_access_ctxt* ctxt, void* arg) {
  return static_cast<Pinetime::Controllers::PraxiomService*>(arg)->OnCommand(ctxt);
}

PraxiomService::PraxiomService() {
}

void PraxiomService::Init() {
  ble_gatts_count_cfg(serviceDefinition);
  ble_gatts_add_svcs(serviceDefinition);
}

int PraxiomService::OnCommand(struct ble_gatt_access_ctxt* ctxt) {
  if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
    // Handle WRITE operation - mobile app sending Bio-Age
    const auto* buffer = ctxt->om;
    const auto* dataBuffer = buffer->om_data;
    
    // Extract 4-byte uint32_t age value
    if (OS_MBUF_PKTLEN(buffer) == sizeof(uint32_t)) {
      basePraxiomAge = ToUInt32(dataBuffer);
      
      NRF_LOG_INFO("Praxiom Base Age received from mobile app: %d", basePraxiomAge);
    } else {
      NRF_LOG_WARNING("Invalid Praxiom Age data length: %d (expected 4)", OS_MBUF_PKTLEN(buffer));
    }
    
    return 0;
  } else if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
    // Handle READ operation - mobile app reading current Bio-Age
    os_mbuf_append(ctxt->om, &basePraxiomAge, sizeof(basePraxiomAge));
    
    NRF_LOG_INFO("Praxiom Base Age read by mobile app: %d", basePraxiomAge);
    
    return 0;
  }
  
  return 0;
}
