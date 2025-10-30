#include "displayapp/screens/PraxiomHealth.h"

#include <cstdio>
#include <chrono>

using namespace Pinetime::Applications::Screens;

namespace {
  constexpr lv_coord_t kLabelSpacing = 32;
}

PraxiomHealth::PraxiomHealth(Controllers::Settings& settingsController, Controllers::DateTime& dateTimeController)
  : settingsController {settingsController}, dateTimeController {dateTimeController} {
  titleLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(titleLabel, "Praxiom Health");
  lv_obj_set_style_local_text_font(titleLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_obj_align(titleLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 10);

  bioAgeLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(bioAgeLabel, titleLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, kLabelSpacing);

  oralLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(oralLabel, bioAgeLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, kLabelSpacing);

  systemicLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(systemicLabel, oralLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, kLabelSpacing);

  fitnessLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(fitnessLabel, systemicLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, kLabelSpacing);

  lastSyncLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(lastSyncLabel, fitnessLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, kLabelSpacing);

  UpdateDisplay();
}

PraxiomHealth::~PraxiomHealth() {
  lv_obj_clean(lv_scr_act());
}

void PraxiomHealth::Refresh() {
  auto data = settingsController.GetPraxiomHealthData();
  if (data.bioAgeDeciYears != lastRenderedData.bioAgeDeciYears || data.oralHealthScore != lastRenderedData.oralHealthScore ||
      data.systemicHealthScore != lastRenderedData.systemicHealthScore || data.fitnessScore != lastRenderedData.fitnessScore ||
      data.lastSyncTimestamp != lastRenderedData.lastSyncTimestamp) {
    UpdateDisplay();
  }
}

void PraxiomHealth::UpdateDisplay() {
  lastRenderedData = settingsController.GetPraxiomHealthData();

  const float bioAgeYears = static_cast<float>(lastRenderedData.bioAgeDeciYears == 0 ? 530 : lastRenderedData.bioAgeDeciYears) / 10.0f;
  lv_label_set_text_fmt(bioAgeLabel, "Bio-Age: %.1f yrs", bioAgeYears);

  lv_label_set_text_fmt(oralLabel, "Oral Health: %u", lastRenderedData.oralHealthScore);
  lv_label_set_text_fmt(systemicLabel, "Systemic Health: %u", lastRenderedData.systemicHealthScore);
  lv_label_set_text_fmt(fitnessLabel, "Fitness: %u", lastRenderedData.fitnessScore);

  const auto lastSyncText = BuildLastSyncLabel(lastRenderedData.lastSyncTimestamp);
  lv_label_set_text(lastSyncLabel, lastSyncText.c_str());
}

std::string PraxiomHealth::BuildLastSyncLabel(uint32_t lastSyncTimestamp) const {
  if (lastSyncTimestamp == 0) {
    return "Last Sync: Never";
  }

  auto now = std::chrono::duration_cast<std::chrono::seconds>(dateTimeController.CurrentDateTime().time_since_epoch()).count();
  if (now <= static_cast<int64_t>(lastSyncTimestamp)) {
    return "Last Sync: <1 min ago";
  }

  auto diff = now - lastSyncTimestamp;
  if (diff < 60) {
    return "Last Sync: <1 min ago";
  } else if (diff < 3600) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "Last Sync: %lld min ago", static_cast<long long>(diff / 60));
    return buffer;
  } else if (diff < 86400) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "Last Sync: %lld h ago", static_cast<long long>(diff / 3600));
    return buffer;
  }

  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "Last Sync: %lld d ago", static_cast<long long>(diff / 86400));
  return buffer;
}
