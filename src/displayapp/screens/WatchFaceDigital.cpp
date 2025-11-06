#include "displayapp/screens/WatchFaceDigital.h"

#include <lvgl/lvgl.h>
#include <cstdio>
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/BleIcon.h"
#include "displayapp/screens/NotificationIcon.h"
#include "displayapp/screens/Symbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"

using namespace Pinetime::Applications::Screens;

namespace {
  void ValueOutOfRange() {
  }
}

WatchFaceDigital::WatchFaceDigital(Controllers::DateTime& dateTimeController,
                                   const Controllers::Battery& batteryController,
                                   const Controllers::Ble& bleController,
                                   Controllers::NotificationManager& notificationManager,
                                   Controllers::Settings& settingsController,
                                   Controllers::HeartRateController& heartRateController,
                                   Controllers::MotionController& motionController)
  : currentDateTime {{}},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController} {

  // Create gradient background (orange to teal)
  static lv_color_t gradientColors[2];
  gradientColors[0] = lv_color_hex(0xFF6B35);  // Orange
  gradientColors[1] = lv_color_hex(0x00CED1);  // Teal
  
  static lv_style_t gradientStyle;
  lv_style_init(&gradientStyle);
  lv_style_set_bg_color(&gradientStyle, LV_STATE_DEFAULT, gradientColors[0]);
  lv_style_set_bg_grad_color(&gradientStyle, LV_STATE_DEFAULT, gradientColors[1]);
  lv_style_set_bg_grad_dir(&gradientStyle, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
  
  lv_obj_t* background = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(background, LV_HOR_RES, LV_VER_RES);
  lv_obj_add_style(background, LV_OBJ_PART_MAIN, &gradientStyle);
  lv_obj_align(background, nullptr, LV_ALIGN_CENTER, 0, 0);

  batteryIcon.Create(lv_scr_act());
  lv_obj_align(batteryIcon.GetObject(), lv_scr_act(), LV_ALIGN_IN_TOP_RIGHT, 0, 0);

  bleIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(bleIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x0082FC));
  lv_label_set_text_static(bleIcon, Symbols::bluetooth);
  lv_obj_align(bleIcon, batteryIcon.GetObject(), LV_ALIGN_OUT_LEFT_MID, -5, 0);

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  // Praxiom Age Label (WHITE for better contrast)
  label_praxiom_age_text = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_praxiom_age_text, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_obj_set_style_local_text_color(label_praxiom_age_text, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_label_set_text_static(label_praxiom_age_text, "Praxiom Age");
  lv_obj_align(label_praxiom_age_text, lv_scr_act(), LV_ALIGN_CENTER, 0, -80);

  // Praxiom Age Number (Large, color-coded)
  label_praxiom_age = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_praxiom_age, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_extrabold_compressed);
  lv_label_set_text_static(label_praxiom_age, "0.0");
  lv_obj_align(label_praxiom_age, lv_scr_act(), LV_ALIGN_CENTER, 0, -30);

  // Time display - UPDATED TO USE jetbrains_mono_42 (GOLDILOCKS SIZE!)
  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_76);
  lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_CENTER, 0, 30);

  label_time_ampm = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_time_ampm, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_label_set_text_static(label_time_ampm, "");
  lv_obj_align(label_time_ampm, lv_scr_act(), LV_ALIGN_IN_RIGHT_MID, -30, -55);

  // Date display
  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_obj_set_style_local_text_font(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_CENTER, 0, 60);
  lv_label_set_text_static(label_date, "");

  // Heart rate
  heartbeatIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(heartbeatIcon, Symbols::heartBeat);
  lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
  lv_obj_align(heartbeatIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
  lv_label_set_text_static(heartbeatValue, "");
  lv_obj_align(heartbeatValue, heartbeatIcon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  // Steps
  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FFE7));
  lv_label_set_text_static(stepValue, "0");
  lv_obj_align(stepValue, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);

  stepIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FFE7));
  lv_label_set_text_static(stepIcon, Symbols::shoe);
  lv_obj_align(stepIcon, stepValue, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceDigital::~WatchFaceDigital() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceDigital::Refresh() {
  powerPresent = batteryController.IsPowerPresent();
  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated() || powerPresent.IsUpdated()) {
    batteryIcon.SetBatteryPercentage(batteryPercentRemaining.Get());
    batteryIcon.SetPowerPresent(powerPresent.Get());
  }

  bleState = bleController.IsConnected();
  bleRadioEnabled = bleController.IsRadioEnabled();
  if (bleState.IsUpdated() || bleRadioEnabled.IsUpdated()) {
    lv_label_set_text_static(bleIcon, BleIcon::GetIcon(bleState.Get()));
  }

  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  currentDateTime = std::chrono::time_point_cast<std::chrono::seconds>(dateTimeController.CurrentDateTime());
  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();

    if (settingsController.GetClockType() == Controllers::Settings::ClockType::H12) {
      char ampmChar[3] = "AM";
      if (hour == 0) {
        hour = 12;
      } else if (hour == 12) {
        ampmChar[0] = 'P';
      } else if (hour > 12) {
        hour = hour - 12;
        ampmChar[0] = 'P';
      }
      lv_label_set_text(label_time_ampm, ampmChar);
      lv_label_set_text_fmt(label_time, "%2d:%02d", hour, minute);
    } else {
      lv_label_set_text_fmt(label_time, "%02d:%02d", hour, minute);
    }

    currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      uint16_t year = dateTimeController.Year();
      uint8_t day = dateTimeController.Day();
      Controllers::DateTime::Months month = dateTimeController.Month();
      Controllers::DateTime::Days dayOfWeek = dateTimeController.DayOfWeek();

      lv_label_set_text_fmt(label_date,
                           "%s %d %s %04d",
                           dateTimeController.DayOfWeekShortToString(dayOfWeek),
                           day,
                           dateTimeController.MonthShortToString(month),
                           year);
    }
  }

  // Update Praxiom Age with color coding and real-time adjustments
  UpdatePraxiomAge();

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
      lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
      lv_label_set_text_fmt(heartbeatValue, "%d", heartbeat.Get());
    } else {
      lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x1B1B1B));
      lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x1B1B1B));
      lv_label_set_text_static(heartbeatValue, "");
    }
  }

  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "%lu", stepCount.Get());
  }
}

void WatchFaceDigital::UpdateBasePraxiomAge(float age) {
  basePraxiomAge = age;
}

void WatchFaceDigital::UpdatePraxiomAge() {
  // Get current sensor data
  uint8_t hr = heartRateController.HeartRate();
  uint32_t steps = motionController.NbSteps();
  
  // Start with base age
  float displayAge = basePraxiomAge;
  
  // Real-time adjustments based on current activity
  if (hr > 0) {
    if (hr > 100) {
      // Elevated heart rate: reduce by 0.5
      displayAge -= 0.5f;
    } else if (hr < 60) {
      // Low heart rate: increase by 0.2
      displayAge += 0.2f;
    }
  }
  
  // Steps adjustment
  if (steps > 10000) {
    displayAge -= 0.3f;  // Good activity level
  } else if (steps < 3000) {
    displayAge += 0.2f;  // Sedentary
  }
  
  // Ensure age stays in reasonable bounds
  if (displayAge < 18.0f) displayAge = 18.0f;
  if (displayAge > 120.0f) displayAge = 120.0f;
  
  // Update the display with decimal point
  lv_label_set_text_fmt(label_praxiom_age, "%.1f", displayAge);
  
  // Color coding based on age
  lv_color_t ageColor;
  if (displayAge < 30.0f) {
    ageColor = lv_color_hex(0x00FF00);  // Green - excellent
  } else if (displayAge < 50.0f) {
    ageColor = LV_COLOR_WHITE;  // White - good
  } else {
    ageColor = lv_color_hex(0xFF0000);  // Red - needs improvement
  }
  
  lv_obj_set_style_local_text_color(label_praxiom_age, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, ageColor);
}
