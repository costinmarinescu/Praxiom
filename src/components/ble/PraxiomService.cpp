#include "components/ble/PraxiomService.h"
#include "components/ble/NimbleController.h"
#include "utility/PraxiomHealth.h"

#include <host/ble_hs.h>
#include <os/os_mbuf.h>
#include <nrf_log.h>

#include <array>
#include <chrono>

using namespace Pinetime::Controllers;

namespace {
  int PraxiomServiceCallback(uint16_t connHandle, uint16_t attrHandle, struct ble_gatt_access_ctxt* ctxt, void* arg) {
    (void)connHandle;
    auto* service = static_cast<PraxiomService*>(arg);
    return service->OnCommand(attrHandle, ctxt);
  }

  void PraxiomTimerCallback(TimerHandle_t timerHandle) {
    auto* service = static_cast<PraxiomService*>(pvTimerGetTimerID(timerHandle));
    if (service != nullptr) {
      service->NotifyCurrentHealthData();
    }
  }
}

Settings* PraxiomService::settings = nullptr;

PraxiomService::PraxiomService(NimbleController& nimble,
                               DateTime& dateTimeController,
                               HeartRateController& heartRateController,
                               MotionController& motionController)
  : nimble {nimble},
    dateTimeController {dateTimeController},
    heartRateController {heartRateController},
    motionController {motionController},
    characteristicDefinition {{.uuid = &bioAgeUuid.u,
                               .access_cb = PraxiomServiceCallback,
                               .arg = this,
                               .flags = BLE_GATT_CHR_F_WRITE,
                               .val_handle = &bioAgeHandle},
                              {.uuid = &packageUuid.u,
                               .access_cb = PraxiomServiceCallback,
                               .arg = this,
                               .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
                               .val_handle = &packageHandle},
                              {0}},
    serviceDefinition {{.type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = &serviceUuid.u, .characteristics = characteristicDefinition},
                       {0}} {
}

void PraxiomService::BindSettings(Settings& settingsController) {
  settings = &settingsController;
}

void PraxiomService::Init() {
  int res = ble_gatts_count_cfg(serviceDefinition);
  ASSERT(res == 0);

  res = ble_gatts_add_svcs(serviceDefinition);
  ASSERT(res == 0);

  periodicTimer = xTimerCreate("PraxiomSync", pdMS_TO_TICKS(180000), pdTRUE, this, PraxiomTimerCallback);
}

int PraxiomService::OnCommand(uint16_t attributeHandle, struct ble_gatt_access_ctxt* ctxt) {
  const uint16_t length = OS_MBUF_PKTLEN(ctxt->om);

  if (attributeHandle == bioAgeHandle) {
    if (length != sizeof(uint16_t)) {
      return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    uint16_t bioAge = 0;
    os_mbuf_copydata(ctxt->om, 0, sizeof(uint16_t), &bioAge);
    HandleBioAgeWrite(bioAge);
    return 0;
  }

  if (attributeHandle == packageHandle) {
    if (length != 5) {
      return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    std::array<uint8_t, 5> buffer {};
    os_mbuf_copydata(ctxt->om, 0, buffer.size(), buffer.data());
    HandlePackageWrite(buffer.data(), buffer.size());
    return 0;
  }

  return BLE_ATT_ERR_UNLIKELY;
}

void PraxiomService::HandleBioAgeWrite(uint16_t ageTenths) {
  if (settings == nullptr) {
    return;
  }

  settings->SetPraxiomBioAge(ageTenths);
  UpdateLastSync();
  settings->SaveSettings();
  NotifyCurrentHealthData();
}

void PraxiomService::HandlePackageWrite(const uint8_t* data, uint16_t length) {
  if (length < 5) {
    return;
  }

  if (settings == nullptr) {
    return;
  }

  const uint16_t bioAge = static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
  const uint8_t oral = data[2];
  const uint8_t systemic = data[3];
  const uint8_t fitness = data[4];

  settings->SetPraxiomBioAge(bioAge);
  settings->SetPraxiomScores(oral, systemic, fitness);
  UpdateLastSync();
  settings->SaveSettings();

  NotifyCurrentHealthData();
}

void PraxiomService::SubscribeNotification(uint16_t attributeHandle) {
  if (attributeHandle == packageHandle) {
    packageNotifyEnabled = true;
    NotifyCurrentHealthData();
  }
}

void PraxiomService::UnsubscribeNotification(uint16_t attributeHandle) {
  if (attributeHandle == packageHandle) {
    packageNotifyEnabled = false;
  }
}

void PraxiomService::NotifyCurrentHealthData() {
  if (!packageNotifyEnabled) {
    return;
  }

  if (settings == nullptr) {
    return;
  }

  uint16_t connectionHandle = nimble.connHandle();
  if (connectionHandle == 0 || connectionHandle == BLE_HS_CONN_HANDLE_NONE) {
    return;
  }

  const auto healthData = settings->GetPraxiomHealthData();
  const uint16_t adjustedAge = Pinetime::Praxiom::ComputeAdjustedBioAge(
    healthData.bioAgeDeciYears,
    heartRateController.HeartRate(),
    motionController.NbSteps());

  const std::array<uint8_t, 5> payload = {static_cast<uint8_t>(adjustedAge & 0xFF),
                                          static_cast<uint8_t>((adjustedAge >> 8) & 0xFF),
                                          healthData.oralHealthScore,
                                          healthData.systemicHealthScore,
                                          healthData.fitnessScore};

  auto* om = ble_hs_mbuf_from_flat(payload.data(), payload.size());
  if (om == nullptr) {
    return;
  }

  ble_gattc_notify_custom(connectionHandle, packageHandle, om);
}

void PraxiomService::OnConnected() {
  if (periodicTimer != nullptr) {
    xTimerStart(periodicTimer, 0);
  }
  NotifyCurrentHealthData();
}

void PraxiomService::OnDisconnected() {
  if (periodicTimer != nullptr) {
    xTimerStop(periodicTimer, 0);
  }
  packageNotifyEnabled = false;
}

void PraxiomService::UpdateLastSync() {
  if (settings == nullptr) {
    return;
  }

  auto now = std::chrono::duration_cast<std::chrono::seconds>(dateTimeController.CurrentDateTime().time_since_epoch());
  settings->SetPraxiomLastSync(static_cast<uint32_t>(now.count()));
}
