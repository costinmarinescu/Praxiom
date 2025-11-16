#include "systemtask/SystemTask.h"

#define min
#define max
#include <host/ble_gap.h>
#include <host/util/util.h>
#include <nimble/hci_common.h>
#undef max
#undef min
#include <hal/nrf_rtc.h>
#include <libraries/gpiote/app_gpiote.h>
#include <libraries/log/nrf_log.h>

#include "BootErrors.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/brightness/BrightnessController.h"
#include "components/datetime/DateTimeController.h"
#include "components/heartrate/HeartRateController.h"
#include "components/fs/FS.h"
#include "drivers/Cst816s.h"
#include "drivers/PinMap.h"
#include "drivers/St7789.h"
#include "drivers/InternalFlash.h"
#include "drivers/SpiMaster.h"
#include "drivers/SpiNorFlash.h"
#include "drivers/TwiMaster.h"
#include "drivers/Hrs3300.h"
#include "drivers/Bma421.h"
#include "main.h"

#include "BootErrors.h"
#include "components/ble/NimbleController.h"

#include <memory>

using namespace Pinetime::System;

namespace {
  static inline bool in_isr() {
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
  }
}

SystemTask::SystemTask(Drivers::SpiMaster& spi,
                       Drivers::St7789& lcd,
                       Pinetime::Drivers::SpiNorFlash& spiNorFlash,
                       Drivers::TwiMaster& twiMaster,
                       Drivers::Cst816S& touchPanel,
                       Components::LittleVgl& lvgl,
                       Controllers::Battery& batteryController,
                       Controllers::Ble& bleController,
                       Controllers::DateTime& dateTimeController,
                       Controllers::TimerController& timerController,
                       Controllers::AlarmController& alarmController,
                       Drivers::Watchdog& watchdog,
                       Pinetime::Controllers::NotificationManager& notificationManager,
                       Pinetime::Controllers::MotorController& motorController,
                       Pinetime::Drivers::Hrs3300& heartRateSensor,
                       Pinetime::Drivers::Bma421& motionSensor,
                       Controllers::Settings& settingsController,
                       Pinetime::Controllers::HeartRateController& heartRateController,
                       Pinetime::Applications::DisplayApp& displayApp,
                       Pinetime::Applications::HeartRateTask& heartRateApp,
                       Pinetime::Controllers::FS& fs,
                       Pinetime::Controllers::TouchHandler& touchHandler,
                       Pinetime::Controllers::ButtonHandler& buttonHandler)
  : spi {spi},
    lcd {lcd},
    spiNorFlash {spiNorFlash},
    twiMaster {twiMaster},
    touchPanel {touchPanel},
    lvgl {lvgl},
    batteryController {batteryController},
    bleController {bleController},
    dateTimeController {dateTimeController},
    timerController {timerController},
    alarmController {alarmController},
    watchdog {watchdog},
    notificationManager {notificationManager},
    motorController {motorController},
    heartRateSensor {heartRateSensor},
    motionSensor {motionSensor},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionSensor},
    displayApp {displayApp},
    heartRateApp {heartRateApp},
    fs {fs},
    touchHandler {touchHandler},
    buttonHandler {buttonHandler},
    systemTasksMsgQueue(xQueueCreate(10, 1)) {
}

void SystemTask::Start() {
  systemTasksMsgQueue = xQueueCreate(10, 1);
  if (pdPASS != xTaskCreate(SystemTask::Process, "MAIN", 350, this, 0, &taskHandle)) {
    APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
  }
}

void SystemTask::Process(void* instance) {
  auto* app = static_cast<SystemTask*>(instance);
  NRF_LOG_INFO("systemtask task started!");
  app->Work();
}

void SystemTask::Work() {
  BootErrors bootError = BootErrors::None;

  watchdog.Setup(7);
  watchdog.Start();
  NRF_LOG_INFO("Last reset reason : %s", Pinetime::Drivers::Watchdog::ResetReasonToString(watchdog.ResetReason()));

  spi.Init();
  spiNorFlash.Init();
  spiNorFlash.Wakeup();
  
  fs.Init();

  nimbleController.reset(new Pinetime::Controllers::NimbleController(*this,
                                                                      bleController,
                                                                      dateTimeController,
                                                                      notificationManager,
                                                                      batteryController,
                                                                      spiNorFlash,
                                                                      heartRateController,
                                                                      motionController,
                                                                      fs,
                                                                      praxiomController));
  nimbleController->Init();
  nimbleController->StartAdvertising();

  lcd.Init();
  
  twiMaster.Init();
  touchPanel.Init();

  batteryController.Init();
  motorController.Init();
  
  motionSensor.SoftReset();
  
  heartRateSensor.Enable();

  displayApp.Start(this);
  displayApp.PushMessage(Pinetime::Applications::Display::Messages::UpdateBleConnection);

  heartRateApp.Start();

  validator.Validate();

  nrf_gpio_cfg_sense_input(PinMap::Button, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);

  measureBatteryTimer = xTimerCreate("measureBattery", batteryMeasurementPeriod, pdTRUE, this, MeasureBatteryTimerCallback);
  xTimerStart(measureBatteryTimer, 0);

  nrf_gpio_cfg_sense_input(PinMap::PowerPresent, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_HIGH);

  idleTimer = xTimerCreate("idleTimer", pdMS_TO_TICKS(settingsController.GetScreenTimeOut() - 5000), pdFALSE, this, IdleTimerCallback);
  dimTimer = xTimerCreate("dimTimer", pdMS_TO_TICKS(5000), pdFALSE, this, DimTimerCallback);
  xTimerStart(idleTimer, 0);

  for (;;) {
    UpdateMotion();

    uint8_t msg;
    if (xQueueReceive(systemTasksMsgQueue, &msg, 100)) {

      Messages message = static_cast<Messages>(msg);
      switch (message) {
        case Messages::UpdateTimeOut:
          ReloadIdleTimer();
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::UpdateTimeOut);
          break;
        case Messages::GoToRunning:
          spi.Wakeup();
          twiMaster.Wakeup();

          nimbleController->StartAdvertising();
          xTimerStart(dimTimer, 0);
          spiNorFlash.Wakeup();
          lcd.Wakeup();

          displayApp.PushMessage(Pinetime::Applications::Display::Messages::GoToRunning);
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::UpdateBleConnection);
          heartRateApp.PushMessage(Pinetime::Applications::HeartRateTask::Messages::WakeUp);

          isSleeping = false;
          isWakingUp = false;
          isDimmed = false;
          break;
        case Messages::TouchEvent: {
          if (isSleeping) {
            auto touchInfo = touchPanel.GetTouchInfo();
            if (touchInfo.isValid and not touchInfo.isTouch) {
              OnTouchEvent();
            }
          }
          ReloadIdleTimer();
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::TouchEvent);
        } break;
        case Messages::ButtonPushed:
          if (isSleeping) {
            OnButtonPushed();
          }
          ReloadIdleTimer();
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::ButtonPushed);
          break;
        case Messages::ButtonLongPressed:
          ReloadIdleTimer();
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::ButtonLongPressed);
          break;
        case Messages::ButtonLongerPressed:
          ReloadIdleTimer();
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::ButtonLongerPressed);
          break;
        case Messages::ButtonDoubleClicked:
          ReloadIdleTimer();
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::ButtonDoubleClicked);
          break;
        case Messages::GoToSleep:
          if (doNotGoToSleep) {
            break;
          }
          isGoingToSleep = true;
          NRF_LOG_INFO("[systemtask] Going to sleep");
          xTimerStop(idleTimer, 0);
          xTimerStop(dimTimer, 0);
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::GoToSleep);
          break;
        case Messages::OnNewTime:
          ReloadIdleTimer();
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::UpdateDateTime);
          if (isBleDiscoveryTimerRunning) {
            bleDiscoveryTimer++;
            if (bleDiscoveryTimer == 3) {
              isBleDiscoveryTimerRunning = false;
              nimbleController->StartDiscovery();
            }
          }
          break;
        case Messages::OnNewNotification:
          if (isSleeping && !isWakingUp) {
            OnNewNotification();
          }
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::NewNotification);
          break;
        case Messages::OnNewCall:
          if (isSleeping && !isWakingUp) {
            OnNewCall();
          }
          motorController.RunForDuration(35);
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::NewNotification);
          break;
        case Messages::BleConnected:
          ReloadIdleTimer();
          isBleDiscoveryTimerRunning = true;
          bleDiscoveryTimer = 0;
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::BleConnected);
          break;
        case Messages::BleFirmwareUpdateStarted:
          doNotGoToSleep = true;
          if (isSleeping && !isWakingUp) {
            OnNewNotification();
          }
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::BleFirmwareUpdateStarted);
          break;
        case Messages::BleFirmwareUpdateFinished:
          doNotGoToSleep = false;
          xTimerStart(dimTimer, 0);
          if (bleController.IsFirmwareUpdating()) {
            bleController.StopFirmwareUpdate();
            displayApp.PushMessage(Pinetime::Applications::Display::Messages::BleFirmwareUpdateFinished);
          } else {
            displayApp.PushMessage(Pinetime::Applications::Display::Messages::Clock);
          }
          break;
        case Messages::StartFileTransfer:
          NRF_LOG_INFO("[systemtask] FS Started");
          doNotGoToSleep = true;
          if (isSleeping && !isWakingUp) {
            OnNewNotification();
          }
          break;
        case Messages::StopFileTransfer:
          NRF_LOG_INFO("[systemtask] FS Stopped");
          doNotGoToSleep = false;
          xTimerStart(dimTimer, 0);
          break;
        case Messages::OnTouchEvent:
          ReloadIdleTimer();
          break;
        case Messages::OnButtonEvent:
          ReloadIdleTimer();
          break;
        case Messages::OnDisplayTaskSleeping:
          if (BootloaderVersion::IsValid()) {
            if (!settingsController.IsWakeUpModeOn(Pinetime::Controllers::Settings::WakeUpMode::RaiseWrist) &&
                !settingsController.IsWakeUpModeOn(Pinetime::Controllers::Settings::WakeUpMode::Shake) && !motionController.GetShakeWakeStatus()) {
              motionSensor.Disable();
            }
          }
          lcd.Sleep();
          spiNorFlash.Sleep();
          spi.Sleep();
          twiMaster.Sleep();
          isSleeping = true;
          isGoingToSleep = false;
          break;
        case Messages::BatteryPercentageUpdated:
          nimbleController->NotifyBatteryLevel(batteryController.PercentRemaining());
          break;
        case Messages::OnPairing:
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::ShowPairingKey);
          break;
        case Messages::SetOffAlarm:
          if (isSleeping && !isWakingUp) {
            OnNewNotification();
          }
          displayApp.PushMessage(Pinetime::Applications::Display::Messages::AlarmTriggered);
          break;
        case Messages::StopRinging:
          motorController.StopRinging();
          break;
        case Messages::MeasureBatteryTimerExpired:
          batteryController.MeasureVoltage();
          break;
        case Messages::BleRadioEnableToggle:
          if (bleController.IsRadioEnabled()) {
            nimbleController->DisableRadio();
          } else {
            nimbleController->EnableRadio();
            nimbleController->StartAdvertising();
          }
          break;
        case Messages::OnChargingEvent:
          motorController.RunForDuration(15);
          ReloadIdleTimer();
          break;
        default:
          break;
      }
    }

    if (touchHandler.GetNewTouchInfo()) {
      auto gesture = touchHandler.GestureGet();
      if (gesture != Pinetime::Drivers::Cst816S::Gestures::None) {
        isCancellingAvoidSleepMode = true;
        ReloadIdleTimer();
      }
    }

    monitor.Process();
    uint32_t systick_counter = nrf_rtc_counter_get(portNRF_RTC_REG);
    dateTimeController.UpdateTime(systick_counter);
    batteryController.Update();

    if (!bleController.IsConnected()) {
      fastAdvCount = 0;
    }
    if (nrf_gpio_pin_read(PinMap::PowerPresent) != isCharging) {
      isCharging = !isCharging;
      PushMessage(Messages::OnChargingEvent);
    }
  }
}

void SystemTask::UpdateMotion() {
  if (isGoingToSleep || isSleeping) {
    return;
  }

  if (!isSleeping) {
    motionController.Update(motionSensor.ReadAccelerometer());
  }

  if (motionController.ShouldShakeWake(settingsController.GetShakeThreshold())) {
    OnShakeWake();
  }
}

void SystemTask::OnButtonPushed() {
  if (isGoingToSleep) {
    return;
  }
  if (!isSleeping) {
    NRF_LOG_INFO("[systemtask] Button pushed");
    PushMessage(Messages::OnButtonEvent);
    displayApp.PushMessage(Pinetime::Applications::Display::Messages::ButtonPushed);
  } else {
    if (!isWakingUp) {
      NRF_LOG_INFO("[systemtask] Button pushed, waking up");
      OnWakeUp();
    }
  }
}

void SystemTask::OnButtonLongPressed() {
  if (!isSleeping) {
    PushMessage(Messages::OnButtonEvent);
    displayApp.PushMessage(Pinetime::Applications::Display::Messages::ButtonLongPressed);
  }
}

void SystemTask::OnButtonLongerPressed() {
  if (!isSleeping) {
    PushMessage(Messages::OnButtonEvent);
    displayApp.PushMessage(Pinetime::Applications::Display::Messages::ButtonLongerPressed);
  }
}

void SystemTask::OnButtonDoubleClicked() {
  if (isSleeping && !isWakingUp) {
    OnWakeUp();
  } else {
    PushMessage(Messages::OnButtonEvent);
    displayApp.PushMessage(Pinetime::Applications::Display::Messages::ButtonDoubleClicked);
  }
}

void SystemTask::OnTouchEvent() {
  if (!isSleeping) {
    PushMessage(Messages::OnTouchEvent);
  } else if (!isWakingUp) {
    if (settingsController.isWakeUpModeOn(Pinetime::Controllers::Settings::WakeUpMode::SingleTap) or
        settingsController.isWakeUpModeOn(Pinetime::Controllers::Settings::WakeUpMode::DoubleTap)) {
      OnWakeUp();
    }
  }
}

void SystemTask::PushMessage(System::Messages msg) {
  if (in_isr()) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(systemTasksMsgQueue, &msg, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  } else {
    xQueueSend(systemTasksMsgQueue, &msg, portMAX_DELAY);
  }
}

void SystemTask::OnWakeUp() {
  isWakingUp = true;
  PushMessage(Messages::GoToRunning);
}

void SystemTask::OnShakeWake() {
  if (settingsController.isWakeUpModeOn(Pinetime::Controllers::Settings::WakeUpMode::Shake)) {
    OnWakeUp();
  }
}

void SystemTask::OnNewNotification() {
  if (settingsController.GetNotificationStatus() == Pinetime::Controllers::Settings::Notification::On) {
    OnWakeUp();
  }
  motorController.RunForDuration(35);
}

void SystemTask::OnNewCall() {
  if (settingsController.GetNotificationStatus() == Pinetime::Controllers::Settings::Notification::On) {
    OnWakeUp();
  }
  motorController.RunForDuration(35);
}

void SystemTask::ReloadIdleTimer() {
  if (isSleeping || isGoingToSleep) {
    return;
  }
  if (isDimmed) {
    displayApp.PushMessage(Pinetime::Applications::Display::Messages::RestoreBrightness);
    isDimmed = false;
  }
  xTimerReset(idleTimer, 0);
  xTimerReset(dimTimer, 0);
}

void SystemTask::OnIdle() {
  if (doNotGoToSleep) {
    return;
  }
  NRF_LOG_INFO("Idle timeout -> Going to sleep");
  PushMessage(Messages::GoToSleep);
}

void SystemTask::OnDim() {
  if (!isDimmed) {
    displayApp.PushMessage(Pinetime::Applications::Display::Messages::DimScreen);
    isDimmed = true;
  }
}
