#pragma once

#include <FreeRTOS.h>
#include <task.h>
#include <drivers/St7789.h>
#include <drivers/SpiMaster.h>
#include <components/gfx/Gfx.h>
#include <bits/unique_ptr.h>
#include <queue.h>
#include "components/brightness/BrightnessController.h"
#include "components/motor/MotorController.h"
#include "components/firmwarevalidator/FirmwareValidator.h"
#include "components/settings/Settings.h"
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/ble/SimpleWeatherService.h"
#include "components/timer/TimerController.h"
#include "components/alarm/AlarmController.h"
#include "touchhandler/TouchHandler.h"
#include "buttonhandler/ButtonHandler.h"
#include "Apps.h"
#include "Messages.h"
#include "DummyLvglBackend.h"

#include "displayapp/LittleVgl.h"

namespace Pinetime {

  namespace System {
    class SystemTask;
  }
  namespace Controllers {
    class HeartRateController;
    class Ble;
    class TouchHandler;
    class ButtonHandler;
    class BrightnessController;
    class MotorController;
    class FS;
    class PraxiomController;
  }

  namespace Applications {
    class DisplayApp {
    public:
      DisplayApp(Drivers::St7789& lcd,
                 Controllers::Battery& batteryController,
                 Controllers::Ble& bleController,
                 Controllers::DateTime& dateTimeController,
                 Drivers::WatchdogView& watchdog,
                 Pinetime::Controllers::NotificationManager& notificationManager,
                 Pinetime::Controllers::HeartRateController& heartRateController,
                 Controllers::Settings& settingsController,
                 Pinetime::Controllers::MotorController& motorController,
                 Pinetime::Controllers::MotionController& motionController,
                 Pinetime::Controllers::TimerController& timerController,
                 Pinetime::Controllers::AlarmController& alarmController,
                 Controllers::BrightnessController& brightnessController,
                 Controllers::TouchHandler& touchHandler,
                 Controllers::ButtonHandler& buttonHandler,
                 Controllers::FS& filesystem,
                 Controllers::PraxiomController& praxiomController);
      void Start(System::SystemTask* systemTask);
      void PushMessage(Display::Messages msg);

      void StartApp(Apps app, Display::FullRefreshDirections direction);

      void SetFullRefresh(Display::FullRefreshDirections direction);

      void Register(Pinetime::System::SystemTask* systemTask);

    private:
      Pinetime::Drivers::St7789& lcd;
      Pinetime::Controllers::Battery& batteryController;
      Pinetime::Controllers::Ble& bleController;
      Pinetime::Controllers::DateTime& dateTimeController;
      Pinetime::Drivers::WatchdogView& watchdog;
      Pinetime::Controllers::NotificationManager& notificationManager;
      Pinetime::Controllers::HeartRateController& heartRateController;
      Pinetime::Controllers::Settings& settingsController;
      Pinetime::Controllers::MotorController& motorController;
      Pinetime::Controllers::MotionController& motionController;
      Pinetime::Controllers::TimerController& timerController;
      Pinetime::Controllers::AlarmController& alarmController;
      Pinetime::Controllers::BrightnessController& brightnessController;
      Pinetime::Controllers::TouchHandler& touchHandler;
      Pinetime::Controllers::ButtonHandler& buttonHandler;
      Pinetime::Controllers::FS& filesystem;
      Pinetime::Controllers::PraxiomController& praxiomController;

      Pinetime::Components::LittleVgl lvgl;
      Pinetime::Controllers::SimpleWeatherService weatherService;

      TaskHandle_t taskHandle;

      System::SystemTask* systemTask = nullptr;
      Apps nextApp = Apps::None;
      Display::FullRefreshDirections nextDirection = Display::FullRefreshDirections::None;
      bool onClockApp = false;

      std::unique_ptr<Screens::Screen> currentScreen;

      QueueHandle_t msgQueue;

      static void Process(void* instance);
      void DisplayLogo();
      void DisplayOtaProgress(uint8_t percent, uint16_t speed);
      void InitHw();
      void Refresh();
      void ReturnApp(Apps app, Display::FullRefreshDirections direction, TouchEvents touchEvent);
      void LoadNewScreen(Apps app, Display::FullRefreshDirections direction);
      void IdleState();
      void PushMessageToSystemTask(Pinetime::System::Messages message);

      lv_style_t* scrollBarStyle;
      lv_style_t* scrollBgStyle;
    };
  }
}
