#pragma once

#include <memory>

#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <heartratetask/HeartRateTask.h>
#include <components/heartrate/HeartRateController.h>

#include "SystemMonitor.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/brightness/BrightnessController.h"
#include "components/motor/MotorController.h"
#include "components/datetime/DateTimeController.h"
#include "components/firmwarevalidator/FirmwareValidator.h"
#include "components/settings/Settings.h"
#include "displayapp/DisplayApp.h"
#include "drivers/Watchdog.h"
#include "drivers/Cst816s.h"
#include "components/motion/MotionController.h"
#include "components/fs/FS.h"
#include "components/praxiom/PraxiomController.h"

#ifdef PINETIME_IS_RECOVERY
  #include "displayapp/DisplayAppRecovery.h"
  #include "displayapp/DummyLvglBackend.h"
#else
  #include "components/settings/Settings.h"
  #include "displayapp/LittleVgl.h"
#endif

#include "drivers/Bma421.h"
#include "drivers/TwiMaster.h"
#include "drivers/Hrs3300.h"
#include "drivers/PinMap.h"

namespace Pinetime {
  namespace System {
    class SystemTask {
    public:
      enum class Messages : uint8_t {
        GoToSleep,
        GoToRunning,
        TouchEvent,
        ButtonPushed,
        ButtonLongPressed,
        ButtonLongerPressed,
        ButtonDoubleClicked,
        Timer,
        BleFirmwareUpdateStarted,
        BleConnected,
        UpdateTimeOut,
        DimScreen,
        RestoreBrightness,
        MeasureBatteryTimerExpired,
        BatteryPercentageUpdated,
        StartFileTransfer,
        StopFileTransfer,
        BleRadioEnableToggle,
        OnChargingEvent
      };

      SystemTask(Drivers::SpiMaster& spi,
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
                 Pinetime::Controllers::ButtonHandler& buttonHandler);

      void Start();
      void PushMessage(Messages msg);

      void OnButtonPushed();
      void OnButtonLongPressed();
      void OnButtonLongerPressed();
      void OnButtonDoubleClicked();
      void OnTouchEvent();

      void OnIdle();

      Pinetime::Controllers::NimbleController& nimble() {
        return *nimbleController;
      }

      Pinetime::Controllers::PraxiomController& GetPraxiomController() {
        return praxiomController;
      }

    private:
      TaskHandle_t taskHandle;

      Pinetime::Drivers::SpiMaster& spi;
      Pinetime::Drivers::St7789& lcd;
      Pinetime::Drivers::SpiNorFlash& spiNorFlash;
      Pinetime::Drivers::TwiMaster& twiMaster;
      Pinetime::Drivers::Cst816S& touchPanel;
      Pinetime::Components::LittleVgl& lvgl;
      Pinetime::Controllers::Battery& batteryController;
      Pinetime::Controllers::Ble& bleController;
      Pinetime::Controllers::DateTime& dateTimeController;
      Pinetime::Controllers::TimerController& timerController;
      Pinetime::Controllers::AlarmController& alarmController;
      Pinetime::Drivers::Watchdog& watchdog;
      Pinetime::Controllers::NotificationManager& notificationManager;
      Pinetime::Controllers::MotorController& motorController;
      Pinetime::Drivers::Hrs3300& heartRateSensor;
      Pinetime::Drivers::Bma421& motionSensor;
      Pinetime::Controllers::Settings& settingsController;
      Pinetime::Controllers::HeartRateController& heartRateController;
      Pinetime::Controllers::MotionController motionController;
      Pinetime::Applications::DisplayApp& displayApp;
      Pinetime::Applications::HeartRateTask& heartRateApp;
      Pinetime::Controllers::FS& fs;
      Pinetime::Controllers::TouchHandler& touchHandler;
      Pinetime::Controllers::ButtonHandler& buttonHandler;
      Pinetime::Controllers::PraxiomController praxiomController;

      QueueHandle_t systemTasksMsgQueue;
      std::atomic<bool> isSleeping {false};
      std::atomic<bool> isGoingToSleep {false};
      std::atomic<bool> isWakingUp {false};
      Pinetime::Drivers::Bma421::AccelRawData accelerometerData;

      std::unique_ptr<Pinetime::Controllers::NimbleController> nimbleController;
      Controllers::BrightnessController brightnessController;
      Controllers::FirmwareValidator validator;
      SystemMonitor monitor;

      static constexpr uint8_t pinSpiSck = PinMap::SpiSck;
      static constexpr uint8_t pinSpiMosi = PinMap::SpiMosi;
      static constexpr uint8_t pinSpiMiso = PinMap::SpiMiso;
      static constexpr uint8_t pinSpiFlashCsn = PinMap::SpiFlashCsn;
      static constexpr uint8_t pinLcdCsn = PinMap::LcdCsn;
      static constexpr uint8_t pinLcdDataCommand = PinMap::LcdDataCommand;

      static void Process(void* instance);
      void Work();
      void ReloadIdleTimer();
      bool isBleDiscoveryTimerRunning = false;
      uint8_t bleDiscoveryTimer = 0;
      TimerHandle_t measureBatteryTimer;
      bool doNotGoToSleep = false;

      void HandleButtonAction(Controllers::ButtonActions action);

#if configUSE_TRACE_FACILITY == 1
      SystemMonitor<FreeRtosMonitor> monitor;
#else
      SystemMonitor<DummyMonitor> monitor;
#endif
    };
  }
}
