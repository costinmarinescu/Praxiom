#pragma once

#include <array>
#include "displayapp/apps/Apps.h"
#include "Controllers.h"
#include "displayapp/screens/Screen.h"
#include "displayapp/screens/Calculator.h"

namespace Pinetime {
  namespace Applications {

    template<> struct AppTraits<Apps::Calculator> {
      static constexpr Apps app = Apps::Calculator;
      // Keep this independent of project-specific Symbols helpers.
      static constexpr const char* icon = "C";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::Calculator(controllers);
      }

      static bool IsAvailable(Controllers::FS& /*fileSystem*/) {
        return true;
      }
    };

  } // namespace Applications
} // namespace Pinetime
