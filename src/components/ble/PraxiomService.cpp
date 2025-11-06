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
#include "components/ble/PraxiomService.h"
#include <nrf_log.h>

using namespace Pinetime::Controllers;

PraxiomService::PraxiomService()
  : characteristicDefinition {
      {
        .uuid = &writeCharUuid.u,
        .access_cb = OnPraxiomWriteCallback,
        .arg = this, // CRITICAL: passes 'this' pointer to static callback
        .flags = BLE_GATT_CHR_F_WRITE
      },
      {
        .uuid = &notifyCharUuid.u,
        .access_cb = nullptr, // Read-only for notifications
        .arg = this,
        .val_handle = &notifyCharHandle,
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY
      },
      {0} // NULL TERMINATOR - DO NOT FORGET
    },
    serviceDefinition {
      {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &serviceUuid.u,
        .characteristics = characteristicDefinition
      },
      {0} // NULL TERMINATOR - DO NOT FORGET
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
    // Extract 4 bytes for uint32_t age value
    if (OS_MBUF_PKTLEN(ctxt->om) == sizeof(uint32_t)) {
      uint32_t receivedAge;
      os_mbuf_copydata(ctxt->om, 0, sizeof(uint32_t), &receivedAge);
      
      basePraxiomAge = receivedAge;
      NRF_LOG_INFO("BasePraxiomAge updated to: %d", basePraxiomAge);
    }
  }
  return 0;
}

void PraxiomService::NotifyPraxiomAge(uint32_t age) {
  // Notify connected clients of the current age
  os_mbuf* om = ble_hs_mbuf_from_flat(&age, sizeof(age));
  if (om != nullptr) {
    ble_gatts_notify_custom(0, notifyCharHandle, om);
    NRF_LOG_INFO("Notified Praxiom Age: %d", age);
  }
}
