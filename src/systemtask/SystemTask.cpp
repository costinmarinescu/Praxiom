#include "systemtask/SystemTask.h"
#include <libraries/log/nrf_log.h>
#include "displayapp/DisplayApp.h"
#include "drivers/Bma421.h"
#include "drivers/PinMap.h"
#include "drivers/Hrs3300.h"

using namespace Pinetime::System;

namespace {
  void IdleTimerCallback(TimerHandle_t xTimer) {
    auto* sysTask = static_cast<SystemTask*>(pvTimerGetTimerID(xTimer));
    sysTask->PushMessage(Messages::OnTimerDone);
  }

  void MeasureBatteryTimerCallback(TimerHandle_t xTimer) {
    auto* sysTask = static_cast<SystemTask*>(pvTimerGetTimerID(xTimer));
    sysTask->PushMessage(Messages::BatteryPercentageUpdated);
  }
}

SystemTask::SystemTask(Drivers::SpiMaster& spi,
                       Pinetime::Drivers::SpiNorFlash& spiNorFlash,
                       Drivers::TwiMaster& twiMaster,
                       Drivers::Cst816S& touchPanel,
                       Controllers::Battery& batteryController,
                       Controllers::Ble& bleController,
                       Controllers::DateTime& dateTimeController,
                       Controllers::StopWatchController& stopWatchController,
                       Controllers::AlarmController& alarmController,
                       Drivers::Watchdog& watchdog,
                       Pinetime::Controllers::NotificationManager& notificationManager,
                       Pinetime::Drivers::Hrs3300& heartRateSensor,
                       Pinetime::Controllers::MotionController& motionController,
                       Pinetime::Drivers::Bma421& motionSensor,
                       Controllers::Settings& settingsController,
                       Pinetime::Controllers::HeartRateController& heartRateController,
                       Pinetime::Applications::DisplayApp& displayApp,
                       Pinetime::Applications::HeartRateTask& heartRateApp,
                       Pinetime::Controllers::FS& fs,
                       Pinetime::Controllers::TouchHandler& touchHandler,
                       Pinetime::Controllers::ButtonHandler& buttonHandler)
  : spi {spi},
    spiNorFlash {spiNorFlash},
    twiMaster {twiMaster},
    touchPanel {touchPanel},
    batteryController {batteryController},
    bleController {bleController},
    dateTimeController {dateTimeController},
    stopWatchController {stopWatchController},
    alarmController {alarmController},
    watchdog {watchdog},
    notificationManager {notificationManager},
    heartRateSensor {heartRateSensor},
    motionSensor {motionSensor},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController},
    displayApp {displayApp},
    heartRateApp {heartRateApp},
    fs {fs},
    touchHandler {touchHandler},
    buttonHandler {buttonHandler},
    nimbleController {*this,
                      bleController,
                      dateTimeController,
                      notificationManager,
                      batteryController,
                      spiNorFlash,
                      heartRateController,
                      motionController,
                      fs} {
  systemTasksMsgQueue = xQueueCreate(10, 1);
}

void SystemTask::Start() {
  xTaskCreate(SystemTask::Process, "systemtask", 350, this, 0, &taskHandle);
}

void SystemTask::Process(void* instance) {
  auto* app = static_cast<SystemTask*>(instance);
  NRF_LOG_INFO("systemtask task started!");
  app->Work();
}

void SystemTask::Work() {
  watchdog.Setup(7);
  watchdog.Start();
  NRF_LOG_INFO("Last reset reason : %s", Pinetime::Drivers::Watchdog::ResetReasonToString(watchdog.ResetReason()));

  // Initialize NimBLE stack
  nimbleController.Init();
  nimbleController.StartAdvertising();
  
  // ✅ CRITICAL FIX: Register all BLE services with DisplayApp
  displayApp.Register(this);
  displayApp.Register(&nimbleController.GetMusicService());
  displayApp.Register(&nimbleController.GetNavigationService());
  displayApp.Register(&nimbleController.GetWeatherService());
  displayApp.Register(&nimbleController);
  
  // ✅ ADDED: Register PraxiomService so WatchFaceDigital can access it
  displayApp.Register(&nimbleController.GetPraxiomService());
  NRF_LOG_INFO("✅ All BLE services registered with DisplayApp, including PraxiomService");

  // Start display app
  displayApp.Start(System::BootErrors::None);

  // Create battery measurement timer
  measureBatteryTimer = xTimerCreate("measureBattery", batteryMeasurementPeriod, pdTRUE, this, MeasureBatteryTimerCallback);
  xTimerStart(measureBatteryTimer, 0);

  // Enable motion sensor
  motionSensor.Init();
  motionSensor.Enable();

  // Main system loop
  while (true) {
    UpdateMotion();

    Messages msg;
    if (xQueueReceive(systemTasksMsgQueue, &msg, 100) == pdTRUE) {
      switch (msg) {
        case Messages::EnableSleeping:
          // Handle sleep enable
          break;
        case Messages::DisableSleeping:
          // Handle sleep disable
          break;
        case Messages::GoToSleep:
          GoToSleep();
          break;
        case Messages::GoToRunning:
          GoToRunning();
          break;
        case Messages::OnNewTime:
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::UpdateDateTime);
          break;
        case Messages::OnNewNotification:
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::NewNotification);
          break;
        case Messages::BleConnected:
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::UpdateBleConnection);
          break;
        case Messages::BleDisconnected:
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::UpdateBleConnection);
          break;
        case Messages::OnTouchEvent:
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::TouchEvent);
          break;
        case Messages::OnButtonEvent:
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::ButtonPushed);
          break;
        case Messages::BatteryPercentageUpdated:
          batteryController.Update();
          nimbleController.NotifyBatteryLevel(batteryController.PercentRemaining());
          break;
        case Messages::OnTimerDone:
          if (state == SystemTaskState::Running) {
            GoToSleep();
          }
          break;
        case Messages::BleRadioEnableToggle:
          if (bleController.IsRadioEnabled()) {
            nimbleController.DisableRadio();
          } else {
            nimbleController.EnableRadio();
          }
          break;
        default:
          break;
      }
    }

    // Feed watchdog
    watchdog.Kick();

    // Monitor battery
    if (batteryController.IsCharging() != batteryController.WasCharging()) {
      displayApp.PushMessage(Pinetime::Applications::Display::Messages::BatteryStateUpdated);
    }
  }
}

void SystemTask::UpdateMotion() {
  if (motionSensor.ShouldWakeUp()) {
    GoToRunning();
  }
  motionController.Update(motionSensor.X(), motionSensor.Y(), motionSensor.Z(), motionSensor.GetNbSteps());
}

void SystemTask::GoToRunning() {
  if (state == SystemTaskState::Sleeping || state == SystemTaskState::AODSleeping) {
    state = SystemTaskState::Running;
    
    // Wake up display
    displayApp.PushMessage(Pinetime::Applications::Display::Messages::GoToRunning);
    
    // Enable sensors
    heartRateSensor.Enable();
    motionSensor.Enable();
    
    NRF_LOG_INFO("System going to Running state");
  }
}

void SystemTask::GoToSleep() {
  if (state == SystemTaskState::Running) {
    state = SystemTaskState::GoingToSleep;
    
    // Notify display
    displayApp.PushMessage(Pinetime::Applications::Display::Messages::GoToSleep);
    
    // Disable sensors to save power
    heartRateSensor.Disable();
    
    state = SystemTaskState::Sleeping;
    NRF_LOG_INFO("System going to Sleep state");
  }
}

void SystemTask::PushMessage(Messages msg) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(systemTasksMsgQueue, &msg, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void SystemTask::HandleButtonAction(Controllers::ButtonActions action) {
  if (state == SystemTaskState::Sleeping) {
    GoToRunning();
  } else {
    displayApp.PushMessage(Pinetime::Applications::Display::Messages::ButtonPushed);
  }
}
