#pragma once

#include "displayapp/screens/Screen.h"
#include "displayapp/screens/Symbols.h"
#include "components/settings/Settings.h"
#include "components/datetime/DateTimeController.h"

#include <lvgl/lvgl.h>
#include <string>

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class PraxiomHealth : public Screen {
      public:
        PraxiomHealth(Controllers::Settings& settingsController, Controllers::DateTime& dateTimeController);
        ~PraxiomHealth() override;

        void Refresh() override;

      private:
        void UpdateDisplay();
        std::string BuildLastSyncLabel(uint32_t lastSyncTimestamp) const;

        Controllers::Settings& settingsController;
        Controllers::DateTime& dateTimeController;

        lv_obj_t* titleLabel = nullptr;
        lv_obj_t* bioAgeLabel = nullptr;
        lv_obj_t* oralLabel = nullptr;
        lv_obj_t* systemicLabel = nullptr;
        lv_obj_t* fitnessLabel = nullptr;
        lv_obj_t* lastSyncLabel = nullptr;

        Controllers::Settings::PraxiomHealthData lastRenderedData {};
      };
    }

    template <>
    struct AppTraits<Apps::PraxiomHealth> {
      static constexpr Apps app = Apps::PraxiomHealth;
      static constexpr const char* icon = Screens::Symbols::shieldAlt;

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::PraxiomHealth(controllers.settingsController, controllers.dateTimeController);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      };
    };
  }
}
