#include "components/ble/PraxiomHealthService.h"
#include "components/ble/NimbleController.h"
#include "systemtask/SystemTask.h"
#include <nrf_log.h>

using namespace Pinetime::Controllers;

constexpr ble_uuid128_t PraxiomHealthService::bioAgeCharUuid;
constexpr ble_uuid128_t PraxiomHealthService::healthDataCharUuid;
constexpr ble_uuid128_t PraxiomHealthService::sensorDataCharUuid;

PraxiomHealthService::PraxiomHealthService(NimbleController& nimble) : nimble {nimble} {
}

void PraxiomHealthService::Init() {
  // Bio-Age characteristic (Write from app)
  characteristicDefinition[0] = {
    .uuid = &bioAgeCharUuid.u,
    .access_cb = OnBioAgeWrite,
    .arg = this,
    .flags = BLE_GATT_CHR_F_WRITE,
    .val_handle = &bioAgeCharHandle
  };
  
  // Health Data Package characteristic (Write from app)
  characteristicDefinition[1] = {
    .uuid = &healthDataCharUuid.u,
    .access_cb = OnHealthDataWrite,
    .arg = this,
    .flags = BLE_GATT_CHR_F_WRITE,
    .val_handle = &healthDataCharHandle
  };
  
  // Sensor Data characteristic (Read by app)
  characteristicDefinition[2] = {
    .uuid = &sensorDataCharUuid.u,
    .access_cb = OnSensorDataRead,
    .arg = this,
    .flags = BLE_GATT_CHR_F_READ,
    .val_handle = &sensorDataCharHandle
  };
  
  characteristicDefinition[3] = {0};
  
  // Service definition with correct UUID reference
  static constexpr ble_uuid128_t serviceUuid {
    .u = {.type = BLE_UUID_TYPE_128},
    .value = BLE_UUID_PRAXIOM_SERVICE
  };
  
  serviceDefinition[0] = {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = &serviceUuid.u,
    .characteristics = characteristicDefinition
  };
  serviceDefinition[1] = {0};
  
  int res = ble_gatts_count_cfg(serviceDefinition);
  ASSERT(res == 0);
  
  res = ble_gatts_add_svcs(serviceDefinition);
  ASSERT(res == 0);
}

uint16_t PraxiomHealthService::ExtractUint16LE(const uint8_t* data) {
  return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

uint32_t PraxiomHealthService::ExtractUint32LE(const uint8_t* data) {
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[3]) << 24);
}

int PraxiomHealthService::OnBioAgeWrite(uint16_t conn_handle,
                                        uint16_t attr_handle,
                                        struct ble_gatt_access_ctxt* ctxt,
                                        void* arg) {
  auto* service = static_cast<PraxiomHealthService*>(arg);
  
  if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
    return BLE_ATT_ERR_UNLIKELY;
  }
  
  if (ctxt->om->om_len != 2) {
    NRF_LOG_ERROR("[PRAXIOM] Invalid Bio-Age length: %d (expected 2)", ctxt->om->om_len);
    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
  }
  
  uint8_t buffer[2];
  os_mbuf_copydata(ctxt->om, 0, 2, buffer);
  
  service->bioAge = ExtractUint16LE(buffer);
  
  NRF_LOG_INFO("[PRAXIOM] Bio-Age updated: %d (%.1f years)", 
               service->bioAge, service->bioAge / 10.0f);
  
  return 0;
}

int PraxiomHealthService::OnHealthDataWrite(uint16_t conn_handle,
                                            uint16_t attr_handle,
                                            struct ble_gatt_access_ctxt* ctxt,
                                            void* arg) {
  auto* service = static_cast<PraxiomHealthService*>(arg);
  
  if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
    return BLE_ATT_ERR_UNLIKELY;
  }
  
  if (ctxt->om->om_len != 5) {
    NRF_LOG_ERROR("[PRAXIOM] Invalid Health Data length: %d (expected 5)", ctxt->om->om_len);
    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
  }
  
  uint8_t buffer[5];
  os_mbuf_copydata(ctxt->om, 0, 5, buffer);
  
  service->bioAge = ExtractUint16LE(buffer);
  service->overallHealthScore = buffer[2];
  service->specificHealthScore = buffer[3];
  service->fitnessLevel = buffer[4];
  
  NRF_LOG_INFO("[PRAXIOM] Health package: Bio-Age=%d, OHS=%d, SHS=%d, Fitness=%d",
               service->bioAge, service->overallHealthScore, 
               service->specificHealthScore, service->fitnessLevel);
  
  return 0;
}

int PraxiomHealthService::OnSensorDataRead(uint16_t conn_handle,
                                           uint16_t attr_handle,
                                           struct ble_gatt_access_ctxt* ctxt,
                                           void* arg) {
  auto* service = static_cast<PraxiomHealthService*>(arg);
  
  if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
    return BLE_ATT_ERR_UNLIKELY;
  }
  
  // Pack sensor data: [HR(1)] [Steps(4)] [Battery(1)]
  uint8_t buffer[6];
  buffer[0] = service->heartRate;
  buffer[1] = service->stepCount & 0xFF;
  buffer[2] = (service->stepCount >> 8) & 0xFF;
  buffer[3] = (service->stepCount >> 16) & 0xFF;
  buffer[4] = (service->stepCount >> 24) & 0xFF;
  buffer[5] = service->batteryLevel;
  
  int res = os_mbuf_append(ctxt->om, buffer, sizeof(buffer));
  
  NRF_LOG_INFO("[PRAXIOM] Sensor data read: HR=%d, Steps=%lu, Battery=%d%%",
               service->heartRate, service->stepCount, service->batteryLevel);
  
  return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}
