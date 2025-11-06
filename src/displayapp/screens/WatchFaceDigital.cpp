#include "displayapp/screens/WatchFaceDigital.h"

#include <lvgl/lvgl.h>
#include <cstdio>
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/NotificationIcon.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/ble/PraxiomService.h"  // ← ADDED
#include "components/datetime/DateTimeController.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"
#include "components/ble/SimpleWeatherService.h"
#include "components/alarm/AlarmController.h"

using namespace Pinetime::Applications::Screens;

namespace {
  void RefreshTaskCallback(lv_task_t* task) {
    auto* screen = static_cast<WatchFaceDigital*>(task->user_data);
    screen->Refresh();
  }
}

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
  : batteryIcon(false),
    statusIcons(batteryController, bleController, alarmController),
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    alarmController {alarmController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController},
    weatherService {weatherService},
    praxiomService {praxiomService} {

  // Create orange-to-teal gradient background
  backgroundGradient = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(backgroundGradient, LV_HOR_RES, LV_VER_RES);
  lv_obj_set_pos(backgroundGradient, 0, 0);
  
  static lv_style_t gradientStyle;
  lv_style_init(&gradientStyle);
  lv_style_set_bg_opa(&gradientStyle, LV_STATE_DEFAULT, LV_OPA_COVER);
  lv_style_set_bg_color(&gradientStyle, LV_STATE_DEFAULT, lv_color_hex(0xCC6600)); // Orange top
  lv_style_set_bg_grad_color(&gradientStyle, LV_STATE_DEFAULT, lv_color_hex(0x008B8B)); // Teal bottom
  lv_style_set_bg_grad_dir(&gradientStyle, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
  lv_obj_add_style(backgroundGradient, LV_OBJ_PART_MAIN, &gradientStyle);

  batteryIcon.Create(lv_scr_act());
  lv_obj_align(batteryIcon.GetObject(), lv_scr_act(), LV_ALIGN_IN_TOP_RIGHT, 0, 0);

  statusIcons.Create();
  lv_obj_align(statusIcons.GetObject(), lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 0, 0);

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_LIME);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  // Praxiom Age Title
  label_praxiom_age_title = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_praxiom_age_title, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_label_set_text_static(label_praxiom_age_title, "Praxiom Age");
  lv_obj_align(label_praxiom_age_title, nullptr, LV_ALIGN_IN_TOP_MID, 0, 30);

  // Praxiom Age Value
  label_praxiom_age_value = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_praxiom_age_value, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_label_set_text_static(label_praxiom_age_value, "45.0");
  lv_obj_align(label_praxiom_age_value, label_praxiom_age_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

  // Time label
  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_extrabold_compressed);
  lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);

  label_time_ampm = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_time_ampm, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_label_set_text_static(label_time_ampm, "");

  // Date label
  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_CENTER, 0, 40);

  // Heart rate
  heartbeatIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(heartbeatIcon, Symbols::heartBeat);
  lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_obj_align(heartbeatIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 5, -2);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_label_set_text_static(heartbeatValue, "");
  lv_obj_align(heartbeatValue, heartbeatIcon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  heartbeatBpm = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(heartbeatBpm, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_label_set_text_static(heartbeatBpm, "BPM");
  lv_obj_align(heartbeatBpm, heartbeatValue, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  // Step counter
  stepIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_label_set_text_static(stepIcon, Symbols::shoe);
  lv_obj_align(stepIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, -5, -2);

  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_label_set_text_static(stepValue, "0");
  lv_obj_align(stepValue, stepIcon, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  // Weather (if needed)
  weatherIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(weatherIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_label_set_text(weatherIcon, "");
  lv_obj_set_hidden(weatherIcon, true);

  temperature = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(temperature, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_label_set_text(temperature, "");
  lv_obj_set_hidden(temperature, true);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceDigital::~WatchFaceDigital() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceDigital::Refresh() {
  statusIcons.Update();
  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated()) {
    auto batteryPercent = batteryPercentRemaining.Get();
    batteryIcon.SetBatteryPercentage(batteryPercent);
  }

  bleState = bleController.IsConnected();
  bleRadioEnabled = bleController.IsRadioEnabled();
  if (bleState.IsUpdated() || bleRadioEnabled.IsUpdated()) {
    // Update BLE status icons handled by statusIcons.Update()
  }

  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  currentDateTime = std::chrono::time_point_cast<std::chrono::minutes>(dateTimeController.CurrentDateTime());
  if (currentDateTime.IsUpdated()) {
    UpdateClock();
    UpdateDate();
  }

  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    UpdateSteps();
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    UpdateHeartBeat();
  }

  currentWeather = weatherService.Current();
  if (currentWeather.IsUpdated()) {
    UpdateWeather();
  }

  // ← CRITICAL BLE INTEGRATION: Read base age from BLE service
  uint32_t bleAge = praxiomService.GetBasePraxiomAge();
  if (bleAge != 0 && bleAge != lastBasePraxiomAge) {
    lastBasePraxiomAge = bleAge;
    UpdateBasePraxiomAge(static_cast<float>(bleAge));
  }
  
  // Update Praxiom Age display every refresh
  UpdatePraxiomAge();
}

void WatchFaceDigital::UpdateClock() {
  uint8_t hour = dateTimeController.Hours();
  uint8_t minute = dateTimeController.Minutes();

  if (displayedHour != hour || displayedMinute != minute) {
    displayedHour = hour;
    displayedMinute = minute;

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
      lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_CENTER, 0, 0);
    } else {
      lv_label_set_text_fmt(label_time, "%02d:%02d", hour, minute);
      lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_CENTER, 0, 0);
    }
  }
}

void WatchFaceDigital::UpdateDate() {
  uint16_t year = dateTimeController.Year();
  Controllers::DateTime::Months month = dateTimeController.Month();
  uint8_t day = dateTimeController.Day();
  Controllers::DateTime::Days dayOfWeek = dateTimeController.DayOfWeek();

  if (displayedYear != year || displayedMonth != month || displayedDayOfWeek != dayOfWeek || displayedDay != day) {
    // FIXED: Added month name to date display
    lv_label_set_text_fmt(label_date,
                          "%s %d %s %d",
                          dateTimeController.DayOfWeekShortToString(),
                          day,
                          dateTimeController.MonthShortToString(),
                          year);
    displayedYear = year;
    displayedMonth = month;
    displayedDay = day;
    displayedDayOfWeek = dayOfWeek;
    lv_obj_realign(label_date);
  }
}

void WatchFaceDigital::UpdateSteps() {
  lv_label_set_text_fmt(stepValue, "%lu", stepCount.Get());
  lv_obj_realign(stepValue);
}

void WatchFaceDigital::UpdateHeartBeat() {
  if (heartbeatRunning.Get()) {
    lv_obj_set_hidden(heartbeatIcon, false);
    lv_obj_set_hidden(heartbeatValue, false);
    lv_obj_set_hidden(heartbeatBpm, false);
    lv_label_set_text_fmt(heartbeatValue, "%d", heartbeat.Get());
    lv_obj_realign(heartbeatIcon);
    lv_obj_realign(heartbeatValue);
    lv_obj_realign(heartbeatBpm);
  } else {
    lv_obj_set_hidden(heartbeatIcon, true);
    lv_obj_set_hidden(heartbeatValue, true);
    lv_obj_set_hidden(heartbeatBpm, true);
  }
}

void WatchFaceDigital::UpdateWeather() {
  auto optCurrentWeather = currentWeather.Get();
  if (optCurrentWeather) {
    // Temperature is a class - use its methods
    int16_t temp;
    char tempUnit = 'C';
    if (settingsController.GetWeatherFormat() == Controllers::Settings::WeatherFormat::Imperial) {
      temp = optCurrentWeather->temperature.Fahrenheit();
      tempUnit = 'F';
    } else {
      temp = optCurrentWeather->temperature.Celsius();
    }
    lv_label_set_text_fmt(temperature, "%d°%c", temp, tempUnit);
    
    // Map icon IDs to Symbols - using only enum values that actually exist
    const char* icon = "?";
    switch (optCurrentWeather->iconId) {
      case Controllers::SimpleWeatherService::Icons::Sun:
        icon = Symbols::sun;
        break;
      case Controllers::SimpleWeatherService::Icons::CloudsSun:
        icon = Symbols::cloudSun;
        break;
      case Controllers::SimpleWeatherService::Icons::Clouds:
        icon = Symbols::cloud;
        break;
      case Controllers::SimpleWeatherService::Icons::BrokenClouds:
        icon = Symbols::cloudSunRain;
        break;
      case Controllers::SimpleWeatherService::Icons::CloudShowerHeavy:
        icon = Symbols::cloudShowersHeavy;
        break;
      case Controllers::SimpleWeatherService::Icons::CloudSunRain:
        icon = Symbols::cloudSunRain;
        break;
      case Controllers::SimpleWeatherService::Icons::Thunderstorm:
        icon = Symbols::cloudShowersHeavy;
        break;
      case Controllers::SimpleWeatherService::Icons::Snow:
        icon = Symbols::snowflake;
        break;
      case Controllers::SimpleWeatherService::Icons::Smog:
        icon = Symbols::smog;
        break;
      default:
        icon = "?";
        break;
    }
    lv_label_set_text(weatherIcon, icon);
    lv_obj_set_hidden(weatherIcon, false);
    lv_obj_set_hidden(temperature, false);
  } else {
    lv_obj_set_hidden(weatherIcon, true);
    lv_obj_set_hidden(temperature, true);
  }
}

void WatchFaceDigital::UpdatePraxiomAge() {
  // Calculate real-time adjustment based on sensors
  float adjustment = CalculateRealTimeAdjustment();
  float displayAge = basePraxiomAge + adjustment;
  
  // ← CRITICAL FIX: Changed from %d to %.1f to show decimal point (e.g., 38.5 instead of 39)
  lv_label_set_text_fmt(label_praxiom_age_value, "%.1f", displayAge);
  
  // Color coding based on difference from base age
  float diff = displayAge - basePraxiomAge;
  if (diff <= -2.0f) {
    // More than 2 years younger - GREEN (excellent health)
    lv_obj_set_style_local_text_color(label_praxiom_age_value, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
  } else if (diff >= 2.0f) {
    // More than 2 years older - RED (poor health)
    lv_obj_set_style_local_text_color(label_praxiom_age_value, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFF0000));
  } else {
    // Within ±2 years - WHITE (neutral/good health)
    lv_obj_set_style_local_text_color(label_praxiom_age_value, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  }
  
  lv_obj_realign(label_praxiom_age_value);
}

float WatchFaceDigital::CalculateRealTimeAdjustment() {
  float adjustment = 0.0f;
  
  // Heart rate adjustment (more granular)
  uint8_t hr = heartRateController.HeartRate();
  if (hr > 0) {
    if (hr < 50 || hr > 90) {
      adjustment += 1.5f;  // Very poor
    } else if (hr >= 50 && hr <= 60) {
      adjustment -= 0.5f;  // Excellent
    } else if (hr >= 61 && hr <= 70) {
      adjustment -= 0.2f;  // Good
    } else if (hr >= 71 && hr <= 80) {
      adjustment += 0.3f;  // Fair
    } else if (hr >= 81 && hr <= 90) {
      adjustment += 0.8f;  // Suboptimal
    }
  }
  
  // Step count adjustment (more granular)
  uint32_t steps = motionController.NbSteps();
  if (steps >= 10000) {
    adjustment -= 0.8f;  // Excellent
  } else if (steps >= 8000) {
    adjustment -= 0.3f;  // Good
  } else if (steps >= 5000) {
    adjustment += 0.2f;  // Moderate
  } else if (steps >= 3000) {
    adjustment += 0.5f;  // Low
  } else {
    adjustment += 1.0f;  // Sedentary
  }
  
  return adjustment;
}

void WatchFaceDigital::UpdateBasePraxiomAge(float newAge) {
  basePraxiomAge = newAge;
  UpdatePraxiomAge();
}
