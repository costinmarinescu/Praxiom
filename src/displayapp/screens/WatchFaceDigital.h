#pragma once

#include <lvgl/lvgl.h>
#include <cstdint>
#include <memory>
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/ble/SimpleWeatherService.h"
#include "components/settings/Settings.h"
#include "displayapp/screens/BatteryIcon.h"
#include "utility/DirtyValue.h"
#include "displayapp/screens/StatusIcons.h"

namespace Pinetime {
  namespace Controllers {
    class HeartRateController;
    class MotionController;
    class PraxiomController;
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
                        Controllers::SimpleWeatherService& weatherService,
                        Controllers::PraxiomController& praxiomController);

        ~WatchFaceDigital() override;

        void Refresh() override;

      private:
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>> currentDateTime {};
        Utility::DirtyValue<uint32_t> stepCount {};
        Utility::DirtyValue<uint8_t> heartbeat {};
        Utility::DirtyValue<bool> heartbeatRunning {};
        Utility::DirtyValue<bool> notificationState {};
        using OptCurrentWeather = std::optional<Pinetime::Controllers::SimpleWeatherService::CurrentWeather>;
        Utility::DirtyValue<OptCurrentWeather> currentWeather {};

        static constexpr const char* PRAXIOM_TITLE = "PRAXIOM AGE";
        static constexpr const char* BIO_AGE_PLACEHOLDER = "--";

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
        Controllers::PraxiomController& praxiomController;

        lv_task_t* taskRefresh;

        uint8_t displayedHour = -1;
        uint8_t displayedMinute = -1;

        uint16_t currentYear = 1970;
        Controllers::DateTime::Months currentMonth = Controllers::DateTime::Months::Unknown;
        Controllers::DateTime::Days currentDayOfWeek = Controllers::DateTime::Days::Unknown;
        uint8_t currentDay = 0;
      };
    }
  }
}
