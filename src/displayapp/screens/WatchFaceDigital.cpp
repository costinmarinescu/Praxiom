#include "displayapp/screens/WatchFaceDigital.h"

#include <lvgl/lvgl.h>
#include <cstdio>
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/BleIcon.h"
#include "displayapp/screens/NotificationIcon.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/WeatherSymbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/ble/PraxiomService.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"
#include <nrf_log.h>

using namespace Pinetime::Applications::Screens;

// Declare the fonts we need
extern lv_font_t jetbrains_mono_bold_20;
extern lv_font_t jetbrains_mono_42;

WatchFaceDigital::WatchFaceDigital(Controllers::DateTime& dateTimeController,
                                   const Controllers::Battery& batteryController,
                                   const Controllers::Ble& bleController,
                                   const Controllers::AlarmController& alarmController,
                                   Controllers::NotificationManager& notificationManager,
                                   Controllers::Settings& settingsController,
                                   Controllers::HeartRateController& heartRateController,
                                   Controllers::MotionController& motionController,
                                   Controllers::SimpleWeatherService& weatherService,
                                   Controllers::PraxiomService& praxiomService)
  : currentDateTime {{}},
    dateTimeController {dateTimeController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController},
    weatherService {weatherService},
    praxiomService {praxiomService},
    statusIcons(batteryController, bleController, alarmController),
    basePraxiomAge(0),
    lastSyncTime(0) {

  NRF_LOG_INFO("🎯 WatchFaceDigital constructor starting");

  // Create Praxiom brand gradient background (Orange/Amber to Teal/Cyan)
  lv_obj_t* background_gradient = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(background_gradient, 240, 240);
  lv_obj_set_pos(background_gradient, 0, 0);
  lv_obj_set_style_local_radius(background_gradient, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_bg_color(background_gradient, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCC6600));
  lv_obj_set_style_local_bg_grad_color(background_gradient, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x008B8B));
  lv_obj_set_style_local_bg_grad_dir(background_gradient, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
  lv_obj_set_style_local_border_width(background_gradient, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);

  statusIcons.Create();
  lv_obj_align(statusIcons.GetObject(), lv_scr_act(), LV_ALIGN_IN_TOP_RIGHT, -8, 0);

  // Create and position Praxiom Age label (text) - WHITE
  labelPraxiomAge = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(labelPraxiomAge, "Praxiom Age");
  lv_obj_set_style_local_text_font(labelPraxiomAge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_obj_set_style_local_text_color(labelPraxiomAge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
  lv_obj_align(labelPraxiomAge, lv_scr_act(), LV_ALIGN_CENTER, 0, -80);

  // Create age number label - TEST: Start with static "88" to verify font works
  labelPraxiomAgeNumber = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(labelPraxiomAgeNumber, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
  lv_label_set_text_static(labelPraxiomAgeNumber, "88");  // TEST value
  lv_obj_set_style_local_text_color(labelPraxiomAgeNumber, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
  lv_obj_align(labelPraxiomAgeNumber, lv_scr_act(), LV_ALIGN_CENTER, 0, -10);

  NRF_LOG_INFO("🎯 Created age label with test value 88");

  // Time label - BLACK
  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
  lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));
  lv_label_set_text_static(label_time, "00:00");
  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_CENTER, 0, 55);

  // Date label - BLACK
  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));
  lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_CENTER, 0, 90);

  // Heart rate icon and label - BLACK
  heartbeatIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(heartbeatIcon, Symbols::heartBeat);
  lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));
  lv_obj_align(heartbeatIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));
  lv_label_set_text_static(heartbeatValue, "");
  lv_obj_align(heartbeatValue, heartbeatIcon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  // Step icon and label - BLACK
  stepIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));
  lv_label_set_text_static(stepIcon, Symbols::shoe);
  lv_obj_align(stepIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);

  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));
  lv_label_set_text_static(stepValue, "0");
  lv_obj_align(stepValue, stepIcon, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  // Notification icon - BLACK
  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  
  NRF_LOG_INFO("🎯 WatchFaceDigital constructor complete");
  
  Refresh();
}

WatchFaceDigital::~WatchFaceDigital() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceDigital::UpdateBasePraxiomAge(int age) {
  basePraxiomAge = age;
  lastSyncTime = dateTimeController.CurrentDateTime().time_since_epoch().count();
}

int WatchFaceDigital::GetCurrentPraxiomAge() {
  return basePraxiomAge;
}

lv_color_t WatchFaceDigital::GetPraxiomAgeColor(int currentAge, int baseAge) {
  (void)currentAge;
  (void)baseAge;
  return lv_color_hex(0xFFFFFF);
}

void WatchFaceDigital::Refresh() {
  static int refreshCount = 0;
  refreshCount++;
  
  statusIcons.Update();

  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  currentDateTime = std::chrono::time_point_cast<std::chrono::minutes>(dateTimeController.CurrentDateTime());

  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();

    // Update time
    lv_label_set_text_fmt(label_time, "%02d:%02d", hour, minute);

    currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      uint16_t year = dateTimeController.Year();
      uint8_t day = dateTimeController.Day();
      lv_label_set_text_fmt(label_date,
                            "%s %d %s %d",
                            dateTimeController.DayOfWeekShortToString(),
                            day,
                            dateTimeController.MonthShortToString(),
                            year);
      lv_obj_realign(label_date);
    }
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_label_set_text_fmt(heartbeatValue, "%d", heartbeat.Get());
    } else {
      lv_label_set_text_static(heartbeatValue, "");
    }
    lv_obj_realign(heartbeatValue);
  }

  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "%lu", stepCount.Get());
    lv_obj_realign(stepValue);
  }

  // 🔍 DIAGNOSTIC Bio-Age update with extensive logging
  static uint32_t lastLoggedAge = 999;
  uint32_t bleAge = praxiomService.GetBasePraxiomAge();
  
  // Log every 60 refreshes (about once per second) OR when value changes
  if (refreshCount % 60 == 0 || bleAge != lastLoggedAge) {
    NRF_LOG_INFO("🔍 Refresh #%d: bleAge=%d, lastLoggedAge=%d", refreshCount, bleAge, lastLoggedAge);
    lastLoggedAge = bleAge;
  }
  
  if (bleAge > 0) {
    int ageToDisplay = static_cast<int>(bleAge);
    
    if (ageToDisplay != basePraxiomAge) {
      NRF_LOG_INFO("✏️ UPDATING DISPLAY: %d -> %d", basePraxiomAge, ageToDisplay);
      basePraxiomAge = ageToDisplay;
      
      // Try lv_label_set_text_fmt (same as time display)
      lv_label_set_text_fmt(labelPraxiomAgeNumber, "%d", basePraxiomAge);
      lv_obj_set_style_local_text_color(labelPraxiomAgeNumber, 
        LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
      lv_obj_realign(labelPraxiomAgeNumber);
      
      NRF_LOG_INFO("✅ Display updated to: %d", basePraxiomAge);
    }
  }
}
