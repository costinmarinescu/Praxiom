#pragma once

#include <FreeRTOS.h>
#include <task.h>
#include <drivers/St7789.h>
#include <drivers/Cst816s.h>
#include <bits/unique_ptr.h>
#include <queue.h>
#include "Apps.h"
#include "LittleVgl.h"
#include "TouchEvents.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/datetime/DateTimeController.h"
#include "components/ble/NotificationManager.h"
#include "components/settings/Settings.h"
#include "displayapp/screens/Screen.h"
#include "components/motor/MotorController.h"
#include "components/motion/MotionController.h"
// #include "components/timer/TimerController.h"
#include "components/alarm/AlarmController.h"
#include "components/brightness/BrightnessController.h"
#include "touchhandler/TouchHandler.h"
#include "buttonhandler/ButtonHandler.h"
#include "components/fs/FS.h"
#include "components/praxiom/PraxiomController.h"

#include "Messages.h"

namespace Pinetime {
  namespace System {
    class SystemTask;
  }
  namespace Applications {
    class DisplayApp {
    public:
      DisplayApp(Drivers::St7789& lcd,
                 Components::LittleVgl& lvgl,
                 Drivers::Cst816S& touchPanel,
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

    private:
      Pinetime::Drivers::St7789& lcd;
      Pinetime::Components::LittleVgl& lvgl;
      Pinetime::Drivers::Cst816S& touchPanel;
      Controllers::Battery& batteryController;
      Controllers::Ble& bleController;
      Controllers::DateTime& dateTimeController;
      Pinetime::Drivers::WatchdogView& watchdog;
      Pinetime::Controllers::NotificationManager& notificationManager;
      Pinetime::Controllers::HeartRateController& heartRateController;
      Controllers::Settings& settingsController;
      Pinetime::Controllers::MotorController& motorController;
      Pinetime::Controllers::MotionController& motionController;
      Pinetime::Controllers::TimerController& timerController;
      Pinetime::Controllers::AlarmController& alarmController;
      Controllers::BrightnessController& brightnessController;
      Controllers::TouchHandler& touchHandler;
      Controllers::ButtonHandler& buttonHandler;
      Controllers::FS& filesystem;
      Controllers::PraxiomController& praxiomController;

      Pinetime::System::SystemTask* systemTask = nullptr;
      TaskHandle_t taskHandle;
      QueueHandle_t msgQueue;

      static constexpr uint8_t queueSize = 10;
      static constexpr uint8_t itemSize = 1;

      std::unique_ptr<Screens::Screen> currentScreen;
      Apps currentApp = Apps::None;
      Apps returnToApp = Apps::None;
      TouchEvents OnTouchEvent();

      void Refresh();
      void ReturnApp(Apps app, FullRefreshDirections direction, TouchEvents touchEvent);
      void LoadScreen(Apps app, DisplayApp::FullRefreshDirections direction);
      void PushMessageToSystemTask(Pinetime::System::Messages message);

      bool isClock = true;

      Pinetime::Controllers::Timer timer;

      void RunningState();
      void IdleState();

      TickType_t lastWakeTime;
      FullRefreshDirections fullRefreshDirection = FullRefreshDirections::None;

      static void Process(void* instance);
      void DisplayLogo(uint16_t color);
      void DisplayOtaProgress(uint8_t percent, uint16_t color);
    };
  }
}
