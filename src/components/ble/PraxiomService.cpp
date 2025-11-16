#include "components/ble/PraxiomService.h"
#include "components/praxiom/PraxiomController.h"
#include <nrf_log.h>

using namespace Pinetime::Controllers;

namespace {
  int PraxiomServiceCallback(uint16_t conn_handle, uint16_t attr_handle, 
                             struct ble_gatt_access_ctxt* ctxt, void* arg) {
    auto* praxiomService = static_cast<PraxiomService*>(arg);
    return praxiomService->OnBioAgeCommand(ctxt);
  }
}

PraxiomService::PraxiomService(PraxiomController& praxiomController) 
  : praxiomController {praxiomController} {
  
  characteristicDefinition[0] = {
    .uuid = &bioAgeCharUuid.u,
    .access_cb = PraxiomServiceCallback,
    .arg = this,
    .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
    .val_handle = &bioAgeHandle
  };
  
  characteristicDefinition[1] = {0};
  
  serviceDefinition[0] = {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = &praxiomServiceUuid.u,
    .characteristics = characteristicDefinition
  };
  
  serviceDefinition[1] = {0};
}

void PraxiomService::Init() {
  int res = ble_gatts_count_cfg(serviceDefinition);
  if (res != 0) {
    NRF_LOG_ERROR("PraxiomService: ble_gatts_count_cfg failed: %d", res);
    return;
  }
  
  res = ble_gatts_add_svcs(serviceDefinition);
  if (res != 0) {
    NRF_LOG_ERROR("PraxiomService: ble_gatts_add_svcs failed: %d", res);
    return;
  }
  
  NRF_LOG_INFO("PraxiomService initialized");
}

int PraxiomService::OnBioAgeCommand(struct ble_gatt_access_ctxt* ctxt) {
  if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
    size_t notifSize = OS_MBUF_PKTLEN(ctxt->om);
    
    if (notifSize == 4) {
      float bioAge;
      ble_hs_mbuf_to_flat(ctxt->om, &bioAge, sizeof(float), nullptr);
      
      NRF_LOG_INFO("PraxiomService: Received bio-age: %d.%d", 
                   (int)bioAge, (int)((bioAge - (int)bioAge) * 10));
      
      praxiomController.SetBioAge(bioAge);
    }
  }
  return 0;
}
