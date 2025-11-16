/*  Copyright (C) 2024 Praxiom Health

    This file is part of Praxiom.
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
  // Initialize with 0
  basePraxiomAge = 0;
  
  NRF_LOG_INFO("🎯 PraxiomService constructed, initial age = %d", basePraxiomAge);
  
  // Characteristic definition
  characteristicDefinition[0] = {
    .uuid = &praxiomBioAgeCharUuid.u,
    .access_cb = PraxiomCallback,
    .arg = this,
    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
    .val_handle = nullptr,
  };
  characteristicDefinition[1] = {0};

  // Service definition
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
  
  NRF_LOG_INFO("🎯 PraxiomService initialized");
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
      
      // ✅ DIAGNOSTIC: Log before and after
      NRF_LOG_INFO("📝 Before: basePraxiomAge = %d", basePraxiomAge);
      basePraxiomAge = receivedAge;
      NRF_LOG_INFO("✅ After: basePraxiomAge = %d (received %d)", basePraxiomAge, receivedAge);
      NRF_LOG_INFO("📊 Raw bytes: [%d, %d, %d, %d]", 
                   dataBuffer[0], dataBuffer[1], dataBuffer[2], dataBuffer[3]);
    } else {
      NRF_LOG_WARNING("⚠️ Invalid data length: %d (expected 4)", OS_MBUF_PKTLEN(buffer));
    }
    
    return 0;
  } else if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
    // Handle READ operation - mobile app reading current Bio-Age
    NRF_LOG_INFO("📤 READ operation: returning basePraxiomAge = %d", basePraxiomAge);
    
    os_mbuf_append(ctxt->om, &basePraxiomAge, sizeof(basePraxiomAge));
    
    return 0;
  }
  
  NRF_LOG_WARNING("⚠️ Unknown operation: %d", ctxt->op);
  return 0;
}
