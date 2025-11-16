#include "displayapp/DisplayApp.h"
#include <libraries/log/nrf_log.h>
#include <displayapp/screens/HeartRate.h>
#include <displayapp/screens/Motion.h>
#include <displayapp/screens/Timer.h>
#include <displayapp/screens/Alarm.h>
#include "displayapp/screens/Tile.h"
#include "displayapp/Apps.h"
#include "displayapp/Messages.h"
#include "systemtask/SystemTask.h"
#include "components/praxiom/PraxiomController.h"

using namespace Pinetime::Applications;

DisplayApp::DisplayApp(Drivers::St7789& lcd,
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
                       Controllers::PraxiomController& praxiomController)
  : lcd {lcd},
    lvgl {lvgl},
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
    timerController {timerController},
    alarmController {alarmController},
    brightnessController {brightnessController},
    touchHandler {touchHandler},
    buttonHandler {buttonHandler},
    filesystem {filesystem},
    praxiomController {praxiomController},
    weatherService(dateTimeController) {
  msgQueue = xQueueCreate(queueSize, itemSize);
}

void DisplayApp::Start(System::SystemTask* systemTask) {
  msgQueue = xQueueCreate(queueSize, itemSize);
  systemTask->PushMessage(System::Messages::OnDisplayTaskSleeping);

  if (pdPASS != xTaskCreate(DisplayApp::Process, "displayapp", 800, this, 0, &taskHandle)) {
    APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
  }
}

void DisplayApp::Process(void* instance) {
  auto* app = static_cast<DisplayApp*>(instance);
  NRF_LOG_INFO("displayapp task started!");
  app->InitHw();

  while (true) {
    app->Refresh();
  }
}

void DisplayApp::InitHw() {
  brightnessController.Init();
  ApplyBrightness();
}

void DisplayApp::Refresh() {
  TickType_t queueTimeout;
  TickType_t delta;

  switch (state) {
    case States::Idle:
      queueTimeout = portMAX_DELAY;
      break;
    case States::Running:
      delta = xTaskGetTickCount() - lastWakeTime;
      if (delta > LV_DISP_DEF_REFR_PERIOD) {
        delta = LV_DISP_DEF_REFR_PERIOD;
      }
      queueTimeout = LV_DISP_DEF_REFR_PERIOD - delta;
      break;
    default:
      queueTimeout = portMAX_DELAY;
      break;
  }

  Messages msg;
  if (xQueueReceive(msgQueue, &msg, queueTimeout) == pdTRUE) {
    switch (msg) {
      case Messages::DimScreen:
        DimScreen();
        break;
      case Messages::RestoreBrightness:
        RestoreBrightness();
        break;
      case Messages::GoToSleep:
        while (brightnessController.Level() != Controllers::BrightnessController::Levels::Off) {
          brightnessController.Lower();
          vTaskDelay(100);
        }
        lcd.Sleep();
        PushMessageToSystemTask(System::Messages::OnDisplayTaskSleeping);
        state = States::Idle;
        break;
      case Messages::GoToRunning:
        lcd.Wakeup();
        ApplyBrightness();
        state = States::Running;
        break;
      case Messages::UpdateTimeOut:
        PushMessageToSystemTask(System::Messages::UpdateTimeOut);
        break;
      case Messages::UpdateBleConnection:
        break;
      case Messages::NewNotification:
        LoadNewScreen(Apps::NotificationsPreview, DisplayApp::FullRefreshDirections::Up);
        break;
      case Messages::TimerDone:
        LoadNewScreen(Apps::Timer, DisplayApp::FullRefreshDirections::Up);
        break;
      case Messages::AlarmTriggered:
        LoadNewScreen(Apps::Alarm, DisplayApp::FullRefreshDirections::None);
        break;
      case Messages::ShowPairingKey:
        LoadNewScreen(Apps::PassKey, DisplayApp::FullRefreshDirections::Up);
        break;
      case Messages::TouchEvent: {
        if (state != States::Running) {
          break;
        }
        auto gesture = touchPanel.GetGesture();
        if (gesture == Pinetime::Drivers::Cst816S::Gestures::DoubleTap &&
            currentApp == Apps::Clock) {
          LoadNewScreen(Apps::QuickSettings, DisplayApp::FullRefreshDirections::RightAnim);
        } else {
          if (currentScreen != nullptr) {
            currentScreen->OnTouchEvent(touchPanel.GetX(), touchPanel.GetY());
          }
        }
      } break;
      case Messages::ButtonPushed:
        if (currentApp == Apps::Clock) {
          PushMessageToSystemTask(System::Messages::GoToSleep);
        } else {
          if (currentScreen != nullptr) {
            currentScreen->OnButtonPushed();
          }
        }
        break;
      case Messages::ButtonLongPressed:
        if (currentScreen != nullptr) {
          currentScreen->OnButtonLongPressed();
        }
        break;
      case Messages::ButtonLongerPressed:
        // Create Tile screen for App selection
        LoadNewScreen(Apps::Launcher, DisplayApp::FullRefreshDirections::Up);
        break;
      case Messages::ButtonDoubleClicked:
        if (currentApp != Apps::Clock) {
          ReturnToApp(Apps::Clock, DisplayApp::FullRefreshDirections::Down, TouchEvents::None);
        }
        break;
      case Messages::BleFirmwareUpdateStarted:
        LoadNewScreen(Apps::FirmwareUpdate, DisplayApp::FullRefreshDirections::Down);
        break;
      case Messages::BleRadioEnableToggle:
        PushMessageToSystemTask(System::Messages::BleRadioEnableToggle);
        break;
      case Messages::Clock:
        LoadNewScreen(Apps::Clock, DisplayApp::FullRefreshDirections::Up);
        break;
    }
  }

  if (state != States::Running) {
    return;
  }
  lastWakeTime = xTaskGetTickCount();
  lvgl.SetFullRefresh(nextApp);
  lvgl.SetFullRefreshDirection(nextDirection);
  lvgl.FlushAndReset();
  
  if (nextApp != Apps::None) {
    LoadNewScreen(nextApp, nextDirection);
    nextApp = Apps::None;
  }

  if (currentScreen != nullptr) {
    currentScreen->Refresh();
  }
}

void DisplayApp::StartApp(Apps app, DisplayApp::FullRefreshDirections direction) {
  nextApp = app;
  nextDirection = direction;
}

void DisplayApp::ReturnApp(Apps app, DisplayApp::FullRefreshDirections direction, TouchEvents touchEvent) {
  returnToApp = app;
  returnDirection = direction;
  returnTouchEvent = touchEvent;
}

void DisplayApp::LoadNewScreen(Apps app, DisplayApp::FullRefreshDirections direction) {
  touchPanel.ResetTouchInfo();
  currentApp = app;
  nextDirection = direction;

  switch (app) {
    case Apps::Launcher:
      currentScreen = std::make_unique<Screens::Tile>(batteryController,
                                                       dateTimeController,
                                                       bleController,
                                                       *this,
                                                       settingsController);
      break;
    case Apps::Clock:
      currentScreen = std::make_unique<Screens::WatchFaceDigital>(dateTimeController,
                                                                   batteryController,
                                                                   bleController,
                                                                   notificationManager,
                                                                   settingsController,
                                                                   heartRateController,
                                                                   motionController,
                                                                   weatherService,
                                                                   praxiomController);
      break;
    case Apps::FirmwareValidation:
      currentScreen = std::make_unique<Screens::FirmwareValidation>(validator);
      break;
    case Apps::FirmwareUpdate:
      currentScreen = std::make_unique<Screens::FirmwareUpdate>(bleController);
      break;
    case Apps::Notifications:
      currentScreen = std::make_unique<Screens::Notifications>(notificationManager,
                                                                systemTask->nimble().alertService(),
                                                                motorController,
                                                                *this);
      break;
    case Apps::NotificationsPreview:
      currentScreen = std::make_unique<Screens::NotificationIcon>(notificationManager,
                                                                   systemTask->nimble().alertService(),
                                                                   motorController,
                                                                   *this);
      break;
    case Apps::Timer:
      currentScreen = std::make_unique<Screens::Timer>(timerController, *this);
      break;
    case Apps::Alarm:
      currentScreen = std::make_unique<Screens::Alarm>(alarmController, settingsController, motorController, *this);
      break;
    case Apps::QuickSettings:
      currentScreen = std::make_unique<Screens::QuickSettings>(*this,
                                                                batteryController,
                                                                dateTimeController,
                                                                brightnessController,
                                                                motorController,
                                                                settingsController);
      break;
    case Apps::Settings:
      currentScreen = std::make_unique<Screens::Settings>(*this, settingsController);
      break;
    case Apps::SettingWatchFace:
      currentScreen = std::make_unique<Screens::SettingWatchFace>(*this, settingsController, filesystem);
      break;
    case Apps::SettingTimeFormat:
      currentScreen = std::make_unique<Screens::SettingTimeFormat>(*this, settingsController);
      break;
    case Apps::SettingWakeUp:
      currentScreen = std::make_unique<Screens::SettingWakeUp>(*this, settingsController);
      break;
    case Apps::SettingDisplay:
      currentScreen = std::make_unique<Screens::SettingDisplay>(*this, settingsController);
      break;
    case Apps::SettingSteps:
      currentScreen = std::make_unique<Screens::SettingSteps>(*this, settingsController);
      break;
    case Apps::SettingSetDateTime:
      currentScreen = std::make_unique<Screens::SettingSetDateTime>(*this, dateTimeController, settingsController);
      break;
    case Apps::SettingChimes:
      currentScreen = std::make_unique<Screens::SettingChimes>(*this, settingsController);
      break;
    case Apps::SettingShakeThreshold:
      currentScreen = std::make_unique<Screens::SettingShakeThreshold>(*this, settingsController, motionController, *systemTask);
      break;
    case Apps::SettingBluetooth:
      currentScreen = std::make_unique<Screens::SettingBluetooth>(*this, settingsController);
      break;
    case Apps::BatteryInfo:
      currentScreen = std::make_unique<Screens::BatteryInfo>(batteryController);
      break;
    case Apps::SysInfo:
      currentScreen = std::make_unique<Screens::SystemInfo>(*this,
                                                             dateTimeController,
                                                             batteryController,
                                                             brightnessController,
                                                             bleController,
                                                             watchdog,
                                                             motionController,
                                                             touchPanel);
      break;
    case Apps::FlashLight:
      currentScreen = std::make_unique<Screens::FlashLight>(*this, *systemTask, brightnessController);
      break;
    case Apps::StopWatch:
      currentScreen = std::make_unique<Screens::StopWatch>(*this, *systemTask);
      break;
    case Apps::Twos:
      currentScreen = std::make_unique<Screens::Twos>();
      break;
    case Apps::Paint:
      currentScreen = std::make_unique<Screens::InfiniPaint>(*this, lvgl);
      break;
    case Apps::Paddle:
      currentScreen = std::make_unique<Screens::Paddle>(*this, lvgl);
      break;
    case Apps::Music:
      currentScreen = std::make_unique<Screens::Music>(*this, systemTask->nimble().music());
      break;
    case Apps::Navigation:
      currentScreen = std::make_unique<Screens::Navigation>(*this, systemTask->nimble().navigation());
      break;
    case Apps::HeartRate:
      currentScreen = std::make_unique<Screens::HeartRate>(*this, heartRateController, *systemTask);
      break;
    case Apps::Metronome:
      currentScreen = std::make_unique<Screens::Metronome>(*this, motorController, *systemTask);
      break;
    case Apps::Motion:
      currentScreen = std::make_unique<Screens::Motion>(*this, motionController);
      break;
    case Apps::Steps:
      currentScreen = std::make_unique<Screens::Steps>(*this, motionController, settingsController);
      break;
    case Apps::PassKey:
      currentScreen = std::make_unique<Screens::PassKey>(*this, bleController.GetPairingKey());
      break;
    case Apps::Weather:
      currentScreen = std::make_unique<Screens::Weather>(*this, weatherService);
      break;
    case Apps::Error:
      currentScreen = std::make_unique<Screens::Error>(*this, bootError);
      break;
  }
  currentScreen->OnShow();
}

void DisplayApp::IdleState() {
}

void DisplayApp::PushMessage(Messages msg) {
  if (in_isr()) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(msgQueue, &msg, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  } else {
    xQueueSend(msgQueue, &msg, portMAX_DELAY);
  }
}

void DisplayApp::PushMessageToSystemTask(Pinetime::System::Messages message) {
  if (systemTask != nullptr) {
    systemTask->PushMessage(message);
  }
}

void DisplayApp::Register(Pinetime::System::SystemTask* systemTask) {
  this->systemTask = systemTask;
}

void DisplayApp::ApplyBrightness() {
  auto brightness = settingsController.GetBrightness();
  if (brightness != Controllers::BrightnessController::Levels::Off) {
    brightnessController.Set(brightness);
  }
}

void DisplayApp::SetFullRefresh(DisplayApp::FullRefreshDirections direction) {
  nextDirection = direction;
}

void DisplayApp::SetTouchMode(DisplayApp::TouchModes mode) {
  touchMode = mode;
}

void DisplayApp::DimScreen() {
  if (brightnessController.Level() != Controllers::BrightnessController::Levels::Low) {
    brightnessLevelBackup = brightnessController.Level();
    brightnessController.Set(Controllers::BrightnessController::Levels::Low);
  }
}

void DisplayApp::RestoreBrightness() {
  if (brightnessController.Level() == Controllers::BrightnessController::Levels::Low) {
    brightnessController.Set(brightnessLevelBackup);
  }
}
