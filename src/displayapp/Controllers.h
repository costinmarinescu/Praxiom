#pragma once

namespace Pinetime {
  namespace System {
    class SystemTask;
  }
  
  namespace Controllers {
    class Battery;
    class Ble;
    class DateTime;
    class NotificationManager;
    class HeartRateController;
    class MotionController;
    class MotorController;
    class Settings;
    class SimpleWeatherService;
    class MusicService;
    class NavigationService;
    class FS;
    class AlarmController;
    class StopWatchController;
    class BrightnessController;
    class Timer;
    class NimbleController;  // Forward declaration for nimbleController member
  }
  
  namespace Components {
    class LittleVgl;
  }

  namespace Applications {
    class DisplayApp;
    
    struct AppControllers {
      // CRITICAL: Order and types MUST EXACTLY match DisplayApp.cpp lines 111-129!
      // DO NOT add or remove members without updating DisplayApp.cpp constructor
      
      const Controllers::Battery& batteryController;              // 1
      const Controllers::Ble& bleController;                      // 2
      Controllers::DateTime& dateTimeController;                  // 3
      Controllers::NotificationManager& notificationManager;      // 4
      Controllers::HeartRateController& heartRateController;      // 5
      Controllers::Settings& settingsController;                  // 6
      Controllers::MotorController& motorController;              // 7
      Controllers::MotionController& motionController;            // 8
      Controllers::StopWatchController& stopWatchController;      // 9
      Controllers::AlarmController& alarmController;              // 10
      Controllers::BrightnessController& brightnessController;    // 11
      Pinetime::System::SystemTask* systemTask;                   // 12 - set by Register()
      Controllers::FS& filesystem;                                // 13
      Controllers::Timer& timer;                                  // 14
      Controllers::MusicService* musicService;                    // 15 - set by Register()
      DisplayApp* displayApp;                                     // 16
      Pinetime::Components::LittleVgl& lvgl;                      // 17
      Controllers::NavigationService* navigationService;          // 18 - set by Register()
      Controllers::SimpleWeatherService* weatherController;       // 19 - set by Register()
      
      // Additional members needed by WatchFaceDigital and other screens:
      // These are POINTERS because they're set via Register() after construction
      Controllers::SimpleWeatherService* weatherService;          // Pointer - dereferenced in WatchFaceDigital
      Controllers::NimbleController* nimbleController;            // Pointer - used in WatchFaceDigital
    };
  }
}
