#pragma once

namespace Pinetime {
  namespace Controllers {
    class Battery;
    class Ble;
    class DateTime;
    class NotificationManager;
    class HeartRateController;
    class MotionController;
    class Settings;
    class SimpleWeatherService;
    class FS;
    class AlarmController;
    class NimbleController;  // ← ADDED forward declaration
  }

  namespace Applications {
    struct AppControllers {
      const Controllers::Battery& batteryController;
      const Controllers::Ble& bleController;
      Controllers::DateTime& dateTimeController;
      Controllers::NotificationManager& notificationManager;
      Controllers::HeartRateController& heartRateController;
      Controllers::MotionController& motionController;
      Controllers::Settings& settingsController;
      Controllers::SimpleWeatherService& weatherService;
      Controllers::FS& filesystem;
      Controllers::AlarmController& alarmController;
      Controllers::NimbleController& nimbleController;  // ← ADDED member
    };
  }
}
