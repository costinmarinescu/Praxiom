#include "components/ble/PraxiomService.h"
#include <nrf_log.h>

using namespace Pinetime::Controllers;

namespace {
  int PraxiomServiceCallback(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt* ctxt, void* arg) {
    auto* praxiomService = static_cast<PraxiomService*>(arg);
    return praxiomService->OnBioAgeWrite(conn_handle, attr_handle, ctxt);
  }
  
  int HealthMetricsCallback(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt* ctxt, void* arg) {
    auto* praxiomService = static_cast<PraxiomService*>(arg);
    return praxiomService->OnHealthMetricsRead(conn_handle, attr_handle, ctxt);
  }
  
  int StatusCallback(uint16_t conn_handle, uint16_t attr_handle,
                    struct ble_gatt_access_ctxt* ctxt, void* arg) {
    auto* praxiomService = static_cast<PraxiomService*>(arg);
    return praxiomService->OnStatusRead(conn_handle, attr_handle, ctxt);
  }
}

PraxiomService::PraxiomService() 
  : bioAge(0.0f),
    bioAgeReceived(false),
    heartRate(0),
    steps(0),
    batteryPercent(0),
    statusFlags(0) {
  
  characteristicDefinition[0] = {
    .uuid = &bioAgeCharBaseUuid.u,
    .access_cb = PraxiomServiceCallback,
    .arg = this,
    .flags = BLE_GATT_CHR_F_WRITE,
    .val_handle = &bioAgeHandle
  };
  
  characteristicDefinition[1] = {
    .uuid = &healthMetricsCharBaseUuid.u,
    .access_cb = HealthMetricsCallback,
    .arg = this,
    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
    .val_handle = &healthMetricsHandle
  };
  
  characteristicDefinition[2] = {
    .uuid = &statusCharBaseUuid.u,
    .access_cb = StatusCallback,
    .arg = this,
    .flags = BLE_GATT_CHR_F_READ,
    .val_handle = &statusHandle
  };
  
  characteristicDefinition[3] = {0};
  
  serviceDefinition[0] = {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = &praxiomServiceBaseUuid.u,
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

int PraxiomService::OnBioAgeWrite(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt* ctxt) {
  if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
    return BLE_ATT_ERR_UNLIKELY;
  }
  
  if (ble_hs_mbuf_to_flat(ctxt->om, &bioAge, sizeof(bioAge), nullptr) != sizeof(bioAge)) {
    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
  }
  
  bioAgeReceived = true;
  statusFlags |= 0x01; // Set bio-age received flag
  
  NRF_LOG_INFO("PraxiomService: Bio-Age received: %d.%d years", 
               (int)bioAge, (int)((bioAge - (int)bioAge) * 10));
  
  return 0;
}

int PraxiomService::OnHealthMetricsRead(uint16_t conn_handle, uint16_t attr_handle,
                                        struct ble_gatt_access_ctxt* ctxt) {
  if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
    return BLE_ATT_ERR_UNLIKELY;
  }
  
  uint8_t buffer[7];
  buffer[0] = heartRate;
  buffer[1] = (steps >> 24) & 0xFF;
  buffer[2] = (steps >> 16) & 0xFF;
  buffer[3] = (steps >> 8) & 0xFF;
  buffer[4] = steps & 0xFF;
  buffer[5] = batteryPercent;
  buffer[6] = statusFlags;
  
  int res = os_mbuf_append(ctxt->om, buffer, sizeof(buffer));
  return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

int PraxiomService::OnStatusRead(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt* ctxt) {
  if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
    return BLE_ATT_ERR_UNLIKELY;
  }
  
  int res = os_mbuf_append(ctxt->om, &statusFlags, sizeof(statusFlags));
  return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

void PraxiomService::UpdateHeartRate(uint8_t hr) {
  heartRate = hr;
}

void PraxiomService::UpdateSteps(uint32_t s) {
  steps = s;
}

void PraxiomService::UpdateBattery(uint8_t bp) {
  batteryPercent = bp;
}

void PraxiomService::NotifyHealthMetrics() {
  uint8_t buffer[7];
  buffer[0] = heartRate;
  buffer[1] = (steps >> 24) & 0xFF;
  buffer[2] = (steps >> 16) & 0xFF;
  buffer[3] = (steps >> 8) & 0xFF;
  buffer[4] = steps & 0xFF;
  buffer[5] = batteryPercent;
  buffer[6] = statusFlags;
  
  ble_gatts_notify_custom(0, healthMetricsHandle, buffer, sizeof(buffer));
}
