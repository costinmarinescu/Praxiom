#pragma once

#include <lvgl/lvgl.h>
#include <cstdint>
#include <memory>
#include "displayapp/screens/Screen.h"
#include "displayapp/screens/BatteryIcon.h"
#include "components/datetime/DateTimeController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/ble/PraxiomService.h"  // ← ADDED
#include "displayapp/widgets/StatusIcons.h"
#include "utility/DirtyValue.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class Ble;
    class NotificationManager;
    class HeartRateController;
    class MotionController;
    class SimpleWeatherService;
    class AlarmController;
    class PraxiomService;  // ← ADDED
  }

  namespace Applications {
    namespace Screens {

      class WatchFaceDigital : public Screen {
      public:
        WatchFaceDigital(Controllers::DateTime& dateTimeController,
                        const Controllers::Battery& batteryController,
                        const Controllers::Ble& bleController,
                        const Controllers::AlarmController& alarmController,
                        Controllers::NotificationManager& notificationManager,
                        Controllers::Settings& settingsController,
                        Controllers::HeartRateController& heartRateController,
                        Controllers::MotionController& motionController,
                        Controllers::SimpleWeatherService& weatherService,
                        Controllers::PraxiomService& praxiomService);  // ← ADDED

        ~WatchFaceDigital() override;

        void Refresh() override;

        // Method to update base Praxiom Age from BLE
        void UpdateBasePraxiomAge(float newAge);

      private:
        uint8_t displayedHour = -1;
        uint8_t displayedMinute = -1;
        uint16_t displayedYear = -1;
        Pinetime::Controllers::DateTime::Months displayedMonth = Pinetime::Controllers::DateTime::Months::Unknown;
        Pinetime::Controllers::DateTime::Days displayedDayOfWeek = Pinetime::Controllers::DateTime::Days::Unknown;
        uint8_t displayedDay = -1;
        uint32_t displayedStepCount = 0;
        uint8_t displayedHeartBeat = 0;
        bool displayedConnected = false;

        Utility::DirtyValue<float> batteryPercentRemaining {};
        Utility::DirtyValue<bool> powerPresent {};
        Utility::DirtyValue<bool> bleState {};
        Utility::DirtyValue<bool> bleRadioEnabled {};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>> currentDateTime {};
        Utility::DirtyValue<uint32_t> stepCount {};
        Utility::DirtyValue<uint8_t> heartbeat {};
        Utility::DirtyValue<bool> heartbeatRunning {};
        Utility::DirtyValue<bool> notificationState {};
        Utility::DirtyValue<std::optional<Pinetime::Controllers::SimpleWeatherService::CurrentWeather>> currentWeather {};

        using days = std::chrono::duration<int32_t, std::ratio<86400>>;
        Utility::DirtyValue<days> currentDate;

        lv_obj_t* backgroundGradient;
        lv_obj_t* label_time;
        lv_obj_t* label_time_ampm;
        lv_obj_t* label_date;
        lv_obj_t* stepValue;
        lv_obj_t* stepIcon;
        lv_obj_t* heartbeatValue;
        lv_obj_t* heartbeatIcon;
        lv_obj_t* heartbeatBpm;
        lv_obj_t* notificationIcon;
        lv_obj_t* weatherIcon;
        lv_obj_t* temperature;

        // Praxiom Age display elements
        lv_obj_t* label_praxiom_age_title;
        lv_obj_t* label_praxiom_age_value;
        
        // Base Praxiom Age (received from Android app)
        float basePraxiomAge = 45.0f;
        uint32_t lastBasePraxiomAge = 0;  // ← ADDED for BLE update tracking
        
        // Real-time adjustment based on sensors
        float currentAdjustment = 0.0f;

        BatteryIcon batteryIcon;
        Widgets::StatusIcons statusIcons;

        Controllers::DateTime& dateTimeController;
        const Controllers::Battery& batteryController;
        const Controllers::Ble& bleController;
        const Controllers::AlarmController& alarmController;  // ← ADDED
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;
        Controllers::HeartRateController& heartRateController;
        Controllers::MotionController& motionController;
        Controllers::SimpleWeatherService& weatherService;
        Controllers::PraxiomService& praxiomService;  // ← ADDED

        lv_task_t* taskRefresh;

        void UpdateClock();
        void UpdateDate();
        void UpdateBleConnection();
        void UpdateBattery();
        void UpdateSteps();
        void UpdateHeartBeat();
        void UpdateWeather();
        void UpdatePraxiomAge();
        void SetBatteryLevel(uint8_t batteryPercent);

        float CalculateRealTimeAdjustment();
      };
    }

    // WatchFace traits for the template system
    template <>
    struct WatchFaceTraits<WatchFace::Digital> {
      static constexpr WatchFace watchFace = WatchFace::Digital;
      static constexpr const char* name = "Digital face";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceDigital(controllers.dateTimeController,
                                            controllers.batteryController,
                                            controllers.bleController,
                                            controllers.alarmController,
                                            controllers.notificationManager,
                                            controllers.settingsController,
                                            controllers.heartRateController,
                                            controllers.motionController,
                                            controllers.weatherService,
                                            controllers.nimbleController.GetPraxiomService());
      }

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
