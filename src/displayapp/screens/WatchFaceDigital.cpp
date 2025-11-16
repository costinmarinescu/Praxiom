#include "displayapp/screens/WatchFaceDigital.h"

#include <lvgl/lvgl.h>
#include <cstdio>
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/BleIcon.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/NotificationIcon.h"
#include "components/settings/Settings.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/battery/BatteryController.h"
#include "components/ble/SimpleWeatherService.h"
#include "components/motion/MotionController.h"

using namespace Pinetime::Applications::Screens;

namespace {
  constexpr int16_t POS_Y_TIME = -40;
  constexpr int16_t POS_Y_PLUS_DATE = 3;
  constexpr int16_t POS_Y_PLUS_WEATHER = -33;
  constexpr int16_t POS_X_WEATHER_ICON = -10;
  constexpr int16_t POS_X_TEMP = 55;
  constexpr int16_t POS_X_PLUS_HUMIDITY = 95;
  constexpr int16_t POS_X_DATE = 0;
  constexpr int16_t POS_X_DAYOFWEEK = -20;
  constexpr int16_t POS_X_PLUS_MONTH = 60;
}

WatchFaceDigital::WatchFaceDigital(Controllers::DateTime& dateTimeController,
                                   const Controllers::Battery& batteryController,
                                   const Controllers::Ble& bleController,
                                   Controllers::NotificationManager& notificationManager,
                                   Controllers::Settings& settingsController,
                                   Controllers::HeartRateController& heartRateController,
                                   Controllers::MotionController& motionController,
                                   Controllers::SimpleWeatherService& weatherService)
  : currentDateTime {{}},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController},
    weatherService {weatherService},
    statusIcons(batteryController, bleController) {

  statusIcons.Create();

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_LIME);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  // Praxiom gradient background
  static lv_style_t style_bg;
  lv_style_init(&style_bg);
  lv_style_set_bg_opa(&style_bg, LV_STATE_DEFAULT, LV_OPA_30);
  lv_style_set_bg_color(&style_bg, LV_STATE_DEFAULT, lv_color_hex(0xFF8C00));
  lv_style_set_bg_grad_color(&style_bg, LV_STATE_DEFAULT, lv_color_hex(0x00CFC1));
  lv_style_set_bg_grad_dir(&style_bg, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);

  lv_obj_t* bg = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_add_style(bg, LV_OBJ_PART_MAIN, &style_bg);
  lv_obj_set_size(bg, 240, 240);
  lv_obj_set_pos(bg, 0, 0);
  lv_obj_move_background(bg);

  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_extrabold_compressed);
  lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_obj_align(label_time, nullptr, LV_ALIGN_CENTER, 0, POS_Y_TIME);

  label_time_ampm = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_time_ampm, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_label_set_text_static(label_time_ampm, "");
  lv_obj_set_style_local_text_color(label_time_ampm, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_obj_align(label_time_ampm, label_time, LV_ALIGN_OUT_RIGHT_TOP, 5, 5);

  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_obj_set_style_local_text_font(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_obj_align(label_date, label_time, LV_ALIGN_OUT_BOTTOM_MID, POS_X_DATE, POS_Y_PLUS_DATE);

  // Praxiom Bio-Age Section
  label_bio_age_title = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_bio_age_title, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_obj_set_style_local_text_font(label_bio_age_title, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_label_set_text_static(label_bio_age_title, "PRAXIOM AGE");
  lv_obj_align(label_bio_age_title, nullptr, LV_ALIGN_CENTER, 0, 60);

  label_bio_age = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_bio_age, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_76);
  lv_obj_set_style_local_text_color(label_bio_age, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
  lv_label_set_text_static(label_bio_age, "--");
  lv_obj_align(label_bio_age, label_bio_age_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

  heartbeatIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(heartbeatIcon, Symbols::heartBeat);
  lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
  lv_obj_align(heartbeatIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
  lv_label_set_text_static(heartbeatValue, "");
  lv_obj_align(heartbeatValue, heartbeatIcon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_label_set_text_static(stepValue, "0");
  lv_obj_align(stepValue, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);

  stepIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_label_set_text_static(stepIcon, Symbols::shoe);
  lv_obj_align(stepIcon, stepValue, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  weatherIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(weatherIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_obj_set_style_local_text_font(weatherIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &fontawesome_weathericons);
  lv_label_set_text(weatherIcon, Symbols::ban);
  lv_obj_align(weatherIcon, label_time, LV_ALIGN_OUT_TOP_MID, POS_X_WEATHER_ICON, POS_Y_PLUS_WEATHER);
  lv_obj_set_auto_realign(weatherIcon, true);

  temperature = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(temperature, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_label_set_text_static(temperature, "");
  lv_obj_align(temperature, label_time, LV_ALIGN_OUT_TOP_MID, POS_X_TEMP, POS_Y_PLUS_WEATHER);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceDigital::~WatchFaceDigital() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceDigital::Refresh() {
  statusIcons.Update();

  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  currentDateTime = std::chrono::time_point_cast<std::chrono::seconds>(dateTimeController.CurrentDateTime());
  if (currentDateTime.IsUpdated()) {
    auto newDateTime = currentDateTime.Get();

    auto dp = date::floor<date::days>(newDateTime);
    auto time = date::make_time(newDateTime - dp);
    auto yearMonthDay = date::year_month_day(dp);

    auto year = static_cast<int>(yearMonthDay.year());
    auto month = static_cast<unsigned>(yearMonthDay.month());
    auto day = static_cast<unsigned>(yearMonthDay.day());
    auto dayOfWeek = static_cast<unsigned>(date::weekday(yearMonthDay).iso_encoding());

    uint8_t hour = time.hours().count();
    uint8_t minute = time.minutes().count();

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
        lv_label_set_text_fmt(label_time, "%2d:%02d", hour, minute);
        lv_label_set_text(label_time_ampm, ampmChar);
      } else {
        lv_label_set_text_fmt(label_time, "%02d:%02d", hour, minute);
      }
    }

    if ((year != currentYear) || (month != currentMonth) || (dayOfWeek != currentDayOfWeek) || (day != currentDay)) {
      if (settingsController.GetClockType() == Controllers::Settings::ClockType::H24) {
        lv_label_set_text_fmt(label_date,
                             "%s %02d %s",
                             dateTimeController.DayOfWeekShortToString(static_cast<Controllers::DateTime::Days>(dayOfWeek)),
                             day,
                             dateTimeController.MonthShortToString(static_cast<Controllers::DateTime::Months>(month)));
      } else {
        lv_label_set_text_fmt(label_date,
                             "%s %s %02d",
                             dateTimeController.DayOfWeekShortToString(static_cast<Controllers::DateTime::Days>(dayOfWeek)),
                             dateTimeController.MonthShortToString(static_cast<Controllers::DateTime::Months>(month)),
                             day);
      }

      currentYear = year;
      currentMonth = month;
      currentDayOfWeek = dayOfWeek;
      currentDay = day;
    }
  }

  // Update Bio-Age display (received via BLE custom characteristic)
  // This will be populated when the mobile app sends the biological age
  // For now, show placeholder until data is received
  // TODO: Integrate with BLE characteristic that receives bio-age from app
  
  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_obj_set_hidden(heartbeatIcon, false);
      lv_obj_set_hidden(heartbeatValue, false);
      lv_label_set_text_fmt(heartbeatValue, "%d", heartbeat.Get());
    } else {
      lv_obj_set_hidden(heartbeatIcon, true);
      lv_obj_set_hidden(heartbeatValue, true);
    }

    lv_obj_realign(heartbeatIcon);
    lv_obj_realign(heartbeatValue);
  }

  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "%lu", stepCount.Get());
    lv_obj_realign(stepValue);
    lv_obj_realign(stepIcon);
  }

  currentWeather = weatherService.Current();
  if (currentWeather.IsUpdated()) {
    auto optCurrentWeather = currentWeather.Get();
    if (optCurrentWeather) {
      int16_t temp = optCurrentWeather->temperature;
      char tempUnit = 'C';
      if (settingsController.GetWeatherFormat() == Controllers::Settings::WeatherFormat::Imperial) {
        temp = Controllers::SimpleWeatherService::CelsiusToFahrenheit(temp);
        tempUnit = 'F';
      }
      temp = temp / 100 + (temp % 100 >= 50 ? 1 : 0);
      lv_label_set_text_fmt(temperature, "%d°%c", temp, tempUnit);
      lv_label_set_text(weatherIcon, Symbols::GetSymbol(optCurrentWeather->iconId));
    } else {
      lv_label_set_text_static(temperature, "");
      lv_label_set_text(weatherIcon, Symbols::ban);
    }
    lv_obj_realign(temperature);
    lv_obj_realign(weatherIcon);
  }
}

// Method to update bio-age from BLE (to be called by BLE service)
void WatchFaceDigital::SetBioAge(float age) {
  if (age > 0) {
    lv_label_set_text_fmt(label_bio_age, "%.1f", age);
    lv_obj_set_style_local_text_color(label_bio_age, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));
  } else {
    lv_label_set_text_static(label_bio_age, "--");
    lv_obj_set_style_local_text_color(label_bio_age, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  }
  lv_obj_realign(label_bio_age);
}
