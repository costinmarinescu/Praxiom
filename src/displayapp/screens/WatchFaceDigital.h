#pragma once

#include <lvgl/lvgl.h>
#include <cstdint>
#include <memory>
#include "displayapp/screens/Screen.h"
#include "displayapp/screens/ScreenList.h"
#include "components/datetime/DateTimeController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "displayapp/screens/BatteryIcon.h"
#include "utility/DirtyValue.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/screens/StatusIcons.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class Ble;
    class NotificationManager;
    class HeartRateController;
    class MotionController;
    class SimpleWeatherService;
  }

  namespace Applications {
    namespace Screens {

      class WatchFaceDigital : public Screen {
      public:
        WatchFaceDigital(Controllers::DateTime& dateTimeController,
                        const Controllers::Battery& batteryController,
                        const Controllers::Ble& bleController,
                        Controllers::NotificationManager& notificationManager,
                        Controllers::Settings& settingsController,
                        Controllers::HeartRateController& heartRateController,
                        Controllers::MotionController& motionController,
                        Controllers::SimpleWeatherService& weatherService);

        ~WatchFaceDigital() override;

        void Refresh() override;
        
        // Method to update bio-age from BLE service
        void SetBioAge(float age);

      private:
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>> currentDateTime {};
        Utility::DirtyValue<uint32_t> stepCount {};
        Utility::DirtyValue<uint8_t> heartbeat {};
        Utility::DirtyValue<bool> heartbeatRunning {};
        Utility::DirtyValue<bool> notificationState {};
        using OptCurrentWeather = std::optional<Pinetime::Controllers::SimpleWeatherService::CurrentWeather>;
        Utility::DirtyValue<OptCurrentWeather> currentWeather {};

        lv_obj_t* label_time;
        lv_obj_t* label_time_ampm;
        lv_obj_t* label_date;
        lv_obj_t* heartbeatIcon;
        lv_obj_t* heartbeatValue;
        lv_obj_t* stepIcon;
        lv_obj_t* stepValue;
        lv_obj_t* notificationIcon;
        lv_obj_t* weatherIcon;
        lv_obj_t* temperature;
        lv_obj_t* label_bio_age_title;
        lv_obj_t* label_bio_age;

        StatusIcons statusIcons;

        Controllers::DateTime& dateTimeController;
        const Controllers::Battery& batteryController;
        const Controllers::Ble& bleController;
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;
        Controllers::HeartRateController& heartRateController;
        Controllers::MotionController& motionController;
        Controllers::SimpleWeatherService& weatherService;

        lv_task_t* taskRefresh;

        uint8_t displayedHour = -1;
        uint8_t displayedMinute = -1;

        uint16_t currentYear = 1970;
        Pinetime::Controllers::DateTime::Months currentMonth = Pinetime::Controllers::DateTime::Months::Unknown;
        Pinetime::Controllers::DateTime::Days currentDayOfWeek = Pinetime::Controllers::DateTime::Days::Unknown;
        uint8_t currentDay = 0;
      };
    }
  }
}
