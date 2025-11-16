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

PraxiomService::PraxiomService() : basePraxiomAge(0) {
  // ✅ CRITICAL: Initialize basePraxiomAge to 0 in constructor initializer list
  
  NRF_LOG_INFO("🎯 PraxiomService constructor - basePraxiomAge = 0");
  
  // ✅ CRITICAL: Setup characteristic definition
  characteristicDefinition[0] = {
    .uuid = &praxiomBioAgeCharUuid.u,
    .access_cb = PraxiomCallback,
    .arg = this,
    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
    .val_handle = nullptr,
  };
  characteristicDefinition[1] = {0};

  // ✅ CRITICAL: Setup service definition
  serviceDefinition[0] = {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = &praxiomServiceUuid.u,
    .characteristics = characteristicDefinition
  };
  serviceDefinition[1] = {0};
}

void PraxiomService::Init() {
  ble_gatts_count_cfg(serviceDefinition);
  ble_gatts_add_svcs(serviceDefinition);
  
  NRF_LOG_INFO("🎯 PraxiomService initialized - current basePraxiomAge = %lu", basePraxiomAge);
}

int PraxiomService::OnCommand(struct ble_gatt_access_ctxt* ctxt) {
  if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
    // Handle WRITE operation - mobile app sending Bio-Age
    const auto* buffer = ctxt->om;
    const auto* dataBuffer = buffer->om_data;
    
    NRF_LOG_INFO("📥 WRITE operation received");
    
    // Extract 4-byte uint32_t age value
    if (OS_MBUF_PKTLEN(buffer) == sizeof(uint32_t)) {
      uint32_t receivedAge = ToUInt32(dataBuffer);
      
      NRF_LOG_INFO("📊 Raw bytes: [%u, %u, %u, %u]", 
                   dataBuffer[0], dataBuffer[1], dataBuffer[2], dataBuffer[3]);
      NRF_LOG_INFO("📊 Converted to uint32: %lu", receivedAge);
      NRF_LOG_INFO("📝 Before: basePraxiomAge = %lu", basePraxiomAge);
      
      // ✅ Store the value (validation happens in WatchFaceDigital)
      basePraxiomAge = receivedAge;
      
      NRF_LOG_INFO("✅ After: basePraxiomAge = %lu", basePraxiomAge);
    } else {
      NRF_LOG_WARNING("⚠️ Invalid Praxiom Age data length: %d (expected 4)", OS_MBUF_PKTLEN(buffer));
    }
    
    return 0;
  } else if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
    // Handle READ operation - mobile app reading current Bio-Age
    NRF_LOG_INFO("📤 READ operation: returning basePraxiomAge = %lu", basePraxiomAge);
    
    os_mbuf_append(ctxt->om, &basePraxiomAge, sizeof(basePraxiomAge));
    
    return 0;
  }
  
  NRF_LOG_WARNING("⚠️ Unknown operation: %d", ctxt->op);
  return 0;
}
