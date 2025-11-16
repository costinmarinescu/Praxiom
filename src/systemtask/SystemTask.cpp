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
          displayApp.PushMessage(Pinetime
