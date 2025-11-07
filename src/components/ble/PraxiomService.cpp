#include "components/ble/PraxiomService.h"
#include <nrf_log.h>

#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_hs.h>
#include <host/ble_gatt.h>
#undef max
#undef min

using namespace Pinetime::Controllers;

// Global callback function (like SimpleWeatherService)
int PraxiomCallback(uint16_t /*conn_handle*/, uint16_t /*attr_handle*/, struct ble_gatt_access_ctxt* ctxt, void* arg) {
  return static_cast<Pinetime::Controllers::PraxiomService*>(arg)->OnCommand(ctxt);
}

PraxiomService::PraxiomService() {
  // Constructor is now minimal - arrays initialized in header
}

void PraxiomService::Init() {
  ble_gatts_count_cfg(serviceDefinition);
  ble_gatts_add_svcs(serviceDefinition);
  
  NRF_LOG_INFO("PraxiomService initialized");
}

int PraxiomService::OnCommand(struct ble_gatt_access_ctxt* ctxt) {
  if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
    // Extract 4 bytes for uint32_t age
    if (OS_MBUF_PKTLEN(ctxt->om) == sizeof(uint32_t)) {
      uint32_t receivedAge = 0;
      os_mbuf_copydata(ctxt->om, 0, sizeof(uint32_t), &receivedAge);
      
      // Sanity check: reasonable age range (18-120)
      if (receivedAge >= 18 && receivedAge <= 120) {
        basePraxiomAge = receivedAge;
        NRF_LOG_INFO("BasePraxiomAge received from app: %lu", basePraxiomAge);
      } else {
        NRF_LOG_WARNING("Received invalid age: %lu (ignoring)", receivedAge);
      }
    }
  }
  return 0;
}

void PraxiomService::SetBasePraxiomAge(uint32_t age) {
  if (age >= 18 && age <= 120) {
    basePraxiomAge = age;
    NRF_LOG_INFO("BasePraxiomAge set to: %lu", basePraxiomAge);
  }
}

uint32_t PraxiomService::GetBasePraxiomAge() const {
  return basePraxiomAge;
}

void PraxiomService::NotifyPraxiomAge(uint32_t age) {
  // Notify connected clients with updated age
  os_mbuf* om = ble_hs_mbuf_from_flat(&age, sizeof(age));
  if (om != nullptr) {
    ble_gattc_notify_custom(0, notifyCharHandle, om);
    NRF_LOG_INFO("PraxiomAge notification sent: %lu", age);
  }
}
