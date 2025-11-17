#pragma once

#include <lvgl/src/lv_core/lv_obj.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "components/ble/SimpleWeatherService.h"
#include "components/ble/BleController.h"
#include "displayapp/widgets/StatusIcons.h"
#include "utility/DirtyValue.h"
#include "displayapp/apps/Apps.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class Ble;
    class AlarmController;
    class NotificationManager;
    class HeartRateController;
    class MotionController;
    class PraxiomService;
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
                         Controllers::SimpleWeatherService& weather,
                         Controllers::PraxiomService& praxiomService);
        ~WatchFaceDigital() override;

        void Refresh() override;

        // Called by BLE service to update base Praxiom Age from phone
        void UpdateBasePraxiomAge(int age);
        
        // Get current Praxiom Age (for sending back to phone if needed)
        int GetCurrentPraxiomAge();

      private:
        uint8_t displayedHour = -1;
        uint8_t displayedMinute = -1;

        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>> currentDateTime {};
        Utility::DirtyValue<uint32_t> stepCount {};
        Utility::DirtyValue<uint8_t> heartbeat {};
        Utility::DirtyValue<bool> heartbeatRunning {};
        Utility::DirtyValue<bool> notificationState {};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::days>> currentDate;

        lv_obj_t* label_time;
        lv_obj_t* label_date;
        lv_obj_t* labelPraxiomAge;          // "Praxiom Age" text label
        lv_obj_t* labelPraxiomAgeNumber;    // Praxiom Age number label
        lv_obj_t* heartbeatIcon;
        lv_obj_t* heartbeatValue;
        lv_obj_t* stepIcon;
        lv_obj_t* stepValue;
        lv_obj_t* notificationIcon;

        Controllers::DateTime& dateTimeController;
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;
        Controllers::HeartRateController& heartRateController;
        Controllers::MotionController& motionController;
        Controllers::SimpleWeatherService& weatherService;
        Controllers::PraxiomService& praxiomService;

        lv_task_t* taskRefresh;
        Widgets::StatusIcons statusIcons;
        
        // Praxiom Age variables
        int basePraxiomAge;      // Biological age from phone app biomarker calculation
        uint64_t lastSyncTime;   // Last sync timestamp
        int lastDisplayedAge;    // ✅ ADDED: Track what we last showed to detect changes
        
        // Helper function to determine display color
        lv_color_t GetPraxiomAgeColor(int currentAge, int baseAge);
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Digital> {
      static constexpr WatchFace watchFace = WatchFace::Digital;
      static constexpr const char* name = "Digital face";

      static Screens::Screen* Create(AppControllers& controllers) {
        // ✅ CRITICAL: Verify praxiomService pointer is not null before creating screen
        if (controllers.praxiomService == nullptr) {
          NRF_LOG_ERROR("❌ ERROR: praxiomService is NULL! Cannot create WatchFaceDigital");
          // Return a fallback or crash - this should never happen if SystemTask registered properly
          return nullptr;
        }
        
        NRF_LOG_INFO("✅ Creating WatchFaceDigital with valid PraxiomService");
        return new Screens::WatchFaceDigital(controllers.dateTimeController,
                                             controllers.batteryController,
                                             controllers.bleController,
                                             controllers.alarmController,
                                             controllers.notificationManager,
                                             controllers.settingsController,
                                             controllers.heartRateController,
                                             controllers.motionController,
                                             *controllers.weatherController,
                                             *controllers.praxiomService);  // ✅ Dereference pointer
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
