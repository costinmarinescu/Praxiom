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

using namespace Pinetime::Applications::Screens;

// Declare the fonts we need
extern lv_font_t jetbrains_mono_bold_20;
extern lv_font_t jetbrains_mono_42;
extern lv_font_t jetbrains_mono_76;  // ✅ LARGER FONT for Praxiom Age

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

  // Create Praxiom brand gradient background
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

  // ✅ BIGGER: Praxiom Age label - WHITE
  labelPraxiomAge = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(labelPraxiomAge, "Praxiom Age");
  lv_obj_set_style_local_text_font(labelPraxiomAge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_obj_set_style_local_text_color(labelPraxiomAge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
  lv_obj_align(labelPraxiomAge, lv_scr_act(), LV_ALIGN_CENTER, 0, -85);  // Moved up slightly

  // ✅ MUCH BIGGER: Age number label - now using 76pt font instead of 42pt
  labelPraxiomAgeNumber = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(labelPraxiomAgeNumber, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_76);  // ← BIGGER!
  lv_label_set_text_static(labelPraxiomAgeNumber, "0");
  lv_obj_set_style_local_text_color(labelPraxiomAgeNumber, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
  lv_obj_align(labelPraxiomAgeNumber, lv_scr_act(), LV_ALIGN_CENTER, 0, -15);  // Adjusted position for larger font

  // Time label - BLACK (stays same size 42pt)
  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
  lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));
  lv_label_set_text_static(label_time, "00:00");
  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_CENTER, 0, 60);  // Moved down slightly to make room

  // Date label - BLACK
  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));
  lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_CENTER, 0, 95);

  // Heart rate - BLACK
  heartbeatIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(heartbeatIcon, Symbols::heartBeat);
  lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));
  lv_obj_align(heartbeatIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));
  lv_label_set_text_static(heartbeatValue, "");
  lv_obj_align(heartbeatValue, heartbeatIcon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  // Steps - BLACK
  stepIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));
  lv_label_set_text_static(stepIcon, Symbols::shoe);
  lv_obj_align(stepIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);

  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));
  lv_label_set_text_static(stepValue, "0");
  lv_obj_align(stepValue, stepIcon, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  // Notification - BLACK
  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000));
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
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
  statusIcons.Update();

  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  currentDateTime = std::chrono::time_point_cast<std::chrono::minutes>(dateTimeController.CurrentDateTime());

  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();
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

  // ✅ Bio-Age update - proper conversion and validation
  uint32_t rawAge = praxiomService.GetBasePraxiomAge();
  
  // Validate transmitted data range (180-1200 = ages 18.0 to 120.0)
  if (rawAge >= 180 && rawAge <= 1200) {
    // Valid transmitted age - convert to display format
    int displayAge = static_cast<int>(rawAge / 10);  // e.g., 593 / 10 = 59
    
    if (displayAge != basePraxiomAge) {
      basePraxiomAge = displayAge;
      lv_label_set_text_fmt(labelPraxiomAgeNumber, "%d", basePraxiomAge);
      lv_obj_set_style_local_text_color(labelPraxiomAgeNumber, 
        LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));  // GREEN when updated
      lv_obj_realign(labelPraxiomAgeNumber);
    }
  } else if (rawAge == 0) {
    // Zero means no data yet - show 0 in white
    if (basePraxiomAge != 0) {
      basePraxiomAge = 0;
      lv_label_set_text_fmt(labelPraxiomAgeNumber, "%d", 0);
      lv_obj_set_style_local_text_color(labelPraxiomAgeNumber, 
        LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));  // WHITE for no data
      lv_obj_realign(labelPraxiomAgeNumber);
    }
  }
  // If rawAge is out of valid transmitted range, don't update display
}
