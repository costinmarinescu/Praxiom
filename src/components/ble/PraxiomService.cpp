#include "components/ble/PraxiomService.h"
#include <nrf_log.h>

#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_hs.h>
#include <host/ble_gatt.h>
#undef max
#undef min

using namespace Pinetime::Controllers;

PraxiomService::PraxiomService()
  : characteristicDefinition {
      {
        // Write characteristic - receives Bio-Age from mobile app
        .uuid = &writeCharUuid.u,
        .access_cb = OnPraxiomWriteCallback,
        .arg = this,  // CRITICAL: passes 'this' pointer to static callback
        .flags = BLE_GATT_CHR_F_WRITE
      },
      {
        // Notify characteristic - sends Bio-Age updates to mobile app
        .uuid = &notifyCharUuid.u,
        .access_cb = nullptr,  // Read-only for notifications
        .arg = this,
        .val_handle = &notifyCharHandle,
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY
      },
      {0}  // NULL TERMINATOR - DO NOT FORGET
    },
    serviceDefinition {
      {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &serviceUuid.u,
        .characteristics = characteristicDefinition
      },
      {0}  // NULL TERMINATOR - DO NOT FORGET
    } {
}

void PraxiomService::Init() {
  int res = ble_gatts_count_cfg(serviceDefinition);
  ASSERT(res == 0);
  
  res = ble_gatts_add_svcs(serviceDefinition);
  ASSERT(res == 0);
  
  NRF_LOG_INFO("PraxiomService initialized");
}

int PraxiomService::OnPraxiomWriteCallback(uint16_t conn_handle,
                                           uint16_t attr_handle,
                                           struct ble_gatt_access_ctxt* ctxt,
                                           void* arg) {
  auto service = static_cast<PraxiomService*>(arg);
  return service->OnPraxiomWrite(conn_handle, attr_handle, ctxt);
}

int PraxiomService::OnPraxiomWrite(uint16_t conn_handle,
                                  uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt* ctxt) {
  if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
    // Extract 4 bytes for uint32_t age
    if (OS_MBUF_PKTLEN(ctxt->om) == sizeof(uint32_t)) {
      uint32_t receivedAge = 0;
      os_mbuf_copydata(ctxt->om, 0, sizeof(uint32_t), &receivedAge);
      basePraxiomAge = receivedAge;
      NRF_LOG_INFO("BasePraxiomAge received from app: %lu", basePraxiomAge);
    }
  }
  return 0;
}

void PraxiomService::SetBasePraxiomAge(uint32_t age) {
  basePraxiomAge = age;
  NRF_LOG_INFO("BasePraxiomAge set to: %lu", basePraxiomAge);
}

uint32_t PraxiomService::GetBasePraxiomAge() const {
  return basePraxiomAge;
}

void PraxiomService::NotifyPraxiomAge(uint32_t age) {
  // Notify connected clients with updated age
  os_mbuf* om = ble_hs_mbuf_from_flat(&age, sizeof(age));
  if (om != nullptr) {
    int rc = ble_gatts_notify_custom(0, notifyCharHandle, om);
    if (rc == 0) {
      NRF_LOG_INFO("PraxiomAge notification sent: %lu", age);
    }
  }
}
