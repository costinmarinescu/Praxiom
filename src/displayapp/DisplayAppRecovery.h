#pragma once

#include <memory>
#include <vector>
#include <utility>

#include "displayapp/screens/Screen.h"
#include "displayapp/screens/ErrorScreen.h"
#include "displayapp/LittleVgl.h"
#include "components/ble/NotificationManager.h"
#include "components/ble/BleController.h"
#include "components/firmwarevalidator/FirmwareValidator.h"
#include "components/motor/MotorController.h"
#include "displayapp/Controllers.h"

namespace Pinetime {
  namespace Drivers {
    class St7789;
    class Cst816S;
    class Watchdog;
  }

  namespace Controllers {
    class Battery;
    class Ble;
    class DateTime;
    class NotificationManager;
    class HeartRateController;
    class MotionController;
    class Settings;
    class SimpleWeatherService;
    class MusicService;
    class NavigationService;
    class PraxiomService;  // *** ADDED FOR PRAXIOM ***
  }

  namespace Components {
    class LittleVgl;
  }

  namespace Applications {
    class DisplayApp {
    public:
      enum class Colors : uint8_t { White, Silver, Gray, Black };

      DisplayApp(Drivers::St7789& lcd,
                 Components::LittleVgl& lvgl,
                 Drivers::Cst816S& touchPanel,
                 Drivers::Watchdog& watchdog,
                 Drivers::SpiMaster& spi,
                 Pinetime::Controllers::Battery& batteryController,
                 Pinetime::Controllers::Ble& bleController,
                 Pinetime::Controllers::DateTime& dateTimeController,
                 Pinetime::Controllers::NotificationManager& notificationManager,
                 Pinetime::Controllers::HeartRateController& heartRateController,
                 Pinetime::Controllers::MotionController& motionController,
                 Pinetime::Controllers::MotorController& motorController,
                 Pinetime::Controllers::Settings& settingsController,
                 Pinetime::Controllers::FirmwareValidator& firmwareValidator);

      ~DisplayApp() noexcept;

      void Start(Pinetime::Controllers::FirmwareValidator::FirmwareUpdateStates updateState);

      void PushMessage(Pinetime::Applications::DisplayApp::Messages msg);

      void Register(Pinetime::System::SystemTask* systemTask);
      void Register(Pinetime::Controllers::SimpleWeatherService* weatherService);
      void Register(Pinetime::Controllers::MusicService* musicService);
      void Register(Pinetime::Controllers::NavigationService* NavigationService);
      void Register(Pinetime::Controllers::NimbleController* nimbleController);
      void Register(Pinetime::Controllers::PraxiomService* praxiomService);  // *** ADDED FOR PRAXIOM ***

    private:
      Drivers::St7789& lcd;
      Components::LittleVgl& lvgl;
      Drivers::Cst816S& touchPanel;
      Drivers::Watchdog& watchdog;
      Drivers::SpiMaster& spi;

      Pinetime::Controllers::Battery& batteryController;
      Pinetime::Controllers::Ble& bleController;
      Pinetime::Controllers::DateTime& dateTimeController;
      Pinetime::Controllers::NotificationManager& notificationManager;
      Pinetime::Controllers::HeartRateController& heartRateController;
      Pinetime::Controllers::MotionController& motionController;
      Pinetime::Controllers::MotorController& motorController;
      Pinetime::Controllers::Settings& settingsController;
      Pinetime::Controllers::FirmwareValidator& firmwareValidator;

      std::unique_ptr<Screen> currentScreen;
      std::unique_ptr<Screen> nextScreen;

      lv_obj_t* statusScreen;

      Controllers::AppControllers controllers;

      bool queuedMessageProcessed = false;
      Messages queuedMessage;

      static void EventHandler(lv_obj_t* obj, lv_event_t event);
      void Refresh();
    };
  }
}
