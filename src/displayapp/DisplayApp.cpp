#include "displayapp/DisplayApp.h"
#include <libraries/log/nrf_log.h>
#include "displayapp/screens/HeartRate.h"
#include "displayapp/screens/Motion.h"
#include "displayapp/screens/Timer.h"
#include "displayapp/screens/Alarm.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/datetime/DateTimeController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "displayapp/screens/ApplicationList.h"
#include "displayapp/screens/FirmwareUpdate.h"
#include "displayapp/screens/FirmwareValidation.h"
#include "displayapp/screens/InfiniPaint.h"
#include "displayapp/screens/Paddle.h"
#include "displayapp/screens/StopWatch.h"
#include "displayapp/screens/Metronome.h"
#include "displayapp/screens/Music.h"
#include "displayapp/screens/Navigation.h"
#include "displayapp/screens/Notifications.h"
#include "displayapp/screens/SystemInfo.h"
#include "displayapp/screens/Tile.h"
#include "displayapp/screens/Twos.h"
#include "displayapp/screens/FlashLight.h"
#include "displayapp/screens/BatteryInfo.h"
#include "displayapp/screens/Steps.h"
#include "displayapp/screens/PassKey.h"
#include "displayapp/screens/Error.h"

#include "drivers/Cst816s.h"
#include "drivers/St7789.h"
#include "drivers/Watchdog.h"
#include "systemtask/SystemTask.h"
#include "systemtask/Messages.h"

#include "displayapp/screens/settings/QuickSettings.h"
#include "displayapp/screens/settings/Settings.h"
#include "displayapp/screens/settings/SettingWatchFace.h"
#include "displayapp/screens/settings/SettingTimeFormat.h"
#include "displayapp/screens/settings/SettingWakeUp.h"
#include "displayapp/screens/settings/SettingDisplay.h"
#include "displayapp/screens/settings/SettingSteps.h"
#include "displayapp/screens/settings/SettingSetDateTime.h"
#include "displayapp/screens/settings/SettingChimes.h"
#include "displayapp/screens/settings/SettingShakeThreshold.h"
#include "displayapp/screens/settings/SettingBluetooth.h"

#include "libs/lv_conf.h"

using namespace Pinetime::Applications;
using namespace Pinetime::Applications::Display;

namespace {
  static inline bool in_isr() {
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
  }

  void DummyLvglFlush(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
    // Dummy flush - does nothing
    lv_disp_flush_ready(drv);
  }
}

DisplayApp::DisplayApp(Drivers::St7789& lcd,
                       const Drivers::Cst816S& touchPanel,
                       const Controllers::Battery& batteryController,
                       const Controllers::Ble& bleController,
                       Controllers::DateTime& dateTimeController,
                       const Drivers::Watchdog& watchdog,
                       Pinetime::Controllers::NotificationManager& notificationManager,
                       Pinetime::Controllers::HeartRateController& heartRateController,
                       Controllers::Settings& settingsController,
                       Pinetime::Controllers::MotorController& motorController,
                       Pinetime::Controllers::MotionController& motionController,
                       Pinetime::Controllers::StopWatchController& stopWatchController,
                       Pinetime::Controllers::AlarmController& alarmController,
                       Pinetime::Controllers::BrightnessController& brightnessController,
                       Pinetime::Controllers::TouchHandler& touchHandler,
                       Pinetime::Controllers::FS& filesystem,
                       Pinetime::Drivers::SpiNorFlash& spiNorFlash)
  : lcd {lcd},
    touchPanel {touchPanel},
    batteryController {batteryController},
    bleController {bleController},
    dateTimeController {dateTimeController},
    watchdog {watchdog},
    notificationManager {notificationManager},
    heartRateController {heartRateController},
    settingsController {settingsController},
    motorController {motorController},
    motionController {motionController},
    stopWatchController {stopWatchController},
    alarmController {alarmController},
    brightnessController {brightnessController},
    touchHandler {touchHandler},
    filesystem {filesystem},
    spiNorFlash {spiNorFlash},
    lvgl {lcd, filesystem},
    // ✅ FIXED: Initialize controllers struct with ALL members in correct order
    controllers {
      batteryController,           // 1
      bleController,                // 2
      dateTimeController,           // 3
      notificationManager,          // 4
      heartRateController,          // 5
      settingsController,           // 6
      motorController,              // 7
      motionController,             // 8
      stopWatchController,          // 9
      alarmController,              // 10
      brightnessController,         // 11
      nullptr,                      // 12 - systemTask (set via Register)
      filesystem,                   // 13
      timer,                        // 14
      nullptr,                      // 15 - musicService (set via Register)
      this,                         // 16 - displayApp
      lvgl,                         // 17
      nullptr,                      // 18 - navigationService (set via Register)
      nullptr,                      // 19 - weatherController (set via Register)
      nullptr,                      // 20 - weatherService (set via Register)
      nullptr,                      // 21 - nimbleController (set via Register)
      nullptr                       // 22 - praxiomService (set via Register) ✅ ADDED
    } {
  msgQueue = xQueueCreate(queueSize, itemSize);

  // Create display task
  xTaskCreate(DisplayApp::Process, "displayapp", 800, this, 0, &taskHandle);
}

void DisplayApp::Start(System::BootErrors error) {
  bootError = error;
  PushMessage(Messages::UpdateDateTime);
}

void DisplayApp::Process(void* instance) {
  auto* app = static_cast<DisplayApp*>(instance);
  NRF_LOG_INFO("displayapp task started!");

  app->Init();

  // Send a dummy notification to unlock the lvgl display driver for the first iteration
  xTaskNotifyGive(xTaskGetCurrentTaskHandle());

  while (true) {
    app->Refresh();
  }
}

void DisplayApp::Init() {
  ApplyBrightness();
  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.user_data = this;
  disp_drv.flush_cb = DummyLvglFlush;
  lv_disp_drv_register(&disp_drv);

  if (bootError == System::BootErrors::TouchController) {
    LoadNewScreen(Apps::Error, FullRefreshDirections::None);
  } else {
    LoadNewScreen(Apps::Clock, FullRefreshDirections::None);
  }
}

void DisplayApp::Refresh() {
  TickType_t sleepTime = CalculateSleepTime();
  
  Messages msg;
  if (xQueueReceive(msgQueue, &msg, sleepTime) == pdTRUE) {
    switch (msg) {
      case Messages::UpdateDateTime:
        // Handle time update
        break;
      case Messages::GoToSleep:
        // Handle sleep
        break;
      case Messages::GoToRunning:
        // Handle wake
        break;
      case Messages::UpdateBleConnection:
        // Handle BLE connection changes
        break;
      case Messages::NewNotification:
        // Handle notifications
        break;
      case Messages::TouchEvent: {
        if (state != States::Running) {
          break;
        }
        auto gesture = GetGesture();
        if (gesture == TouchEvents::None) {
          break;
        }
        if (!currentScreen->OnTouchEvent(gesture)) {
          if (currentApp == Apps::Clock) {
            switch (gesture) {
              case TouchEvents::SwipeUp:
                LoadNewScreen(Apps::Launcher, FullRefreshDirections::Up);
                break;
              case TouchEvents::SwipeDown:
                LoadNewScreen(Apps::Notifications, FullRefreshDirections::Down);
                break;
              case TouchEvents::SwipeRight:
                LoadNewScreen(Apps::QuickSettings, FullRefreshDirections::RightAnim);
                break;
              case TouchEvents::DoubleTap:
                PushMessageToSystemTask(System::Messages::GoToSleep);
                break;
              default:
                break;
            }
          } else if (returnTouchEvent == gesture) {
            LoadPreviousScreen();
          }
        } else {
          touchHandler.CancelTap();
        }
      } break;
      case Messages::ButtonPushed:
        if (currentApp == Apps::Clock) {
          PushMessageToSystemTask(System::Messages::GoToSleep);
        } else {
          if (!currentScreen->OnButtonPushed()) {
            LoadPreviousScreen();
          }
        }
        break;
      case Messages::BleFirmwareUpdateStarted:
        LoadNewScreen(Apps::FirmwareUpdate, FullRefreshDirections::Down);
        break;
      case Messages::BleRadioEnableToggle:
        PushMessageToSystemTask(System::Messages::BleRadioEnableToggle);
        break;
      case Messages::UpdateTimeOut:
        PushMessageToSystemTask(System::Messages::UpdateTimeOut);
        break;
      default:
        break;
    }
  }

  if (nextApp != Apps::None) {
    LoadScreen(nextApp, nextDirection);
    nextApp = Apps::None;
  }

  if (state != States::Idle && state != States::AOD) {
    lv_task_handler();
  }
}

void DisplayApp::StartApp(Apps app, DisplayApp::FullRefreshDirections direction) {
  nextApp = app;
  nextDirection = direction;
}

void DisplayApp::LoadScreen(Apps app, DisplayApp::FullRefreshDirections direction) {
  // Screen loading logic
  currentApp = app;
  
  returnAppStack.Push(currentApp);
  appStackDirections.Push(direction);
  
  LoadNewScreen(app, direction);
}

void DisplayApp::LoadNewScreen(Apps app, DisplayApp::FullRefreshDirections direction) {
  // Implementation of screen loading
  currentScreen.reset(nullptr);
  SetFullRefresh(direction);

  switch (app) {
    case Apps::Launcher:
      currentScreen = std::make_unique<Screens::ApplicationList>(controllers,
                                                                   Screens::ApplicationList::Mode::Apps,
                                                                   Screens::Tile::Apps,
                                                                   settingsController);
      break;
    case Apps::Clock:
      currentScreen = Screens::Screen::FullScreenRefresh(settingsController.GetClockFace(), controllers);
      break;
    default:
      currentScreen = std::make_unique<Screens::ApplicationList>(controllers,
                                                                   Screens::ApplicationList::Mode::Apps,
                                                                   Screens::Tile::Apps,
                                                                   settingsController);
  }
  currentApp = app;
}

void DisplayApp::LoadPreviousScreen() {
  Apps previousApp = returnAppStack.Pop();
  FullRefreshDirections previousDirection = appStackDirections.Pop();
  LoadNewScreen(previousApp, previousDirection);
}

void DisplayApp::PushMessage(Display::Messages msg) {
  if (in_isr()) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(msgQueue, &msg, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  } else {
    xQueueSend(msgQueue, &msg, portMAX_DELAY);
  }
}

TouchEvents DisplayApp::GetGesture() {
  auto gesture = touchHandler.GestureGet();
  touchHandler.CancelTap();
  return gesture;
}

void DisplayApp::SetFullRefresh(DisplayApp::FullRefreshDirections direction) {
  // Set full refresh direction
}

void DisplayApp::PushMessageToSystemTask(Pinetime::System::Messages message) {
  if (systemTask != nullptr) {
    systemTask->PushMessage(message);
  }
}

void DisplayApp::Register(Pinetime::System::SystemTask* systemTask) {
  controllers.systemTask = systemTask;
  this->systemTask = systemTask;
}

void DisplayApp::Register(Pinetime::Controllers::SimpleWeatherService* weatherService) {
  controllers.weatherService = weatherService;
  controllers.weatherController = weatherService;
}

void DisplayApp::Register(Pinetime::Controllers::MusicService* musicService) {
  controllers.musicService = musicService;
}

void DisplayApp::Register(Pinetime::Controllers::NavigationService* navigationService) {
  controllers.navigationService = navigationService;
}

void DisplayApp::Register(Pinetime::Controllers::NimbleController* nimbleController) {
  controllers.nimbleController = nimbleController;
}

// ✅ CRITICAL FIX: Added PraxiomService registration method
void DisplayApp::Register(Pinetime::Controllers::PraxiomService* praxiomService) {
  controllers.praxiomService = praxiomService;
  NRF_LOG_INFO("✅ PraxiomService registered in DisplayApp");
}

void DisplayApp::ApplyBrightness() {
  auto brightness = settingsController.GetBrightness();
  brightnessController.Set(brightness);
}

TickType_type DisplayApp::CalculateSleepTime() {
  if (state == States::Running) {
    return pdMS_TO_TICKS(100);
  }
  return portMAX_DELAY;
}
