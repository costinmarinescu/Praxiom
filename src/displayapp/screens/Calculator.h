#pragma once

#include "displayapp/screens/Screen.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "Symbols.h"

namespace Pinetime::Applications {

// If you previously had helper functions like powi() in an anonymous namespace,
// prefer to move them into a detail namespace to avoid impacting template specialization linkage.
namespace detail {
  constexpr int64_t powi(int64_t base, uint8_t exponent) {
    int64_t value = 1;
    while (exponent) {
      value *= base;
      --exponent;
    }
    return value;
  }
}

// Explicit specialization for the Calculator app description/traits.
// Keep this specialization in the SAME namespace as the primary template
// (i.e., Pinetime::Applications) to avoid specialization lookup issues.
template<>
struct AppTraits<Apps::Calculator> {
  static constexpr Apps app = Apps::Calculator;

  // Use a fully-qualified path to the calculator icon symbol so it resolves regardless
  // of using-directives in translation units that include this file.
  static constexpr const char* icon = Pinetime::Applications::Screens::Symbols::calculator;

  // Signature must match what AppDescription expects:
  //   Screens::Screen* (*create)(AppControllers&)
  // We provide a safe stub to guarantee the build; replace with your real screen.
  static Screens::Screen* Create(AppControllers& /*controllers*/ ) {
    return nullptr; // TODO: replace with `return new Screens::Calculator(controllers);`
  }

  // Many apps use a parameterless availability function in their traits.
  // If your AppDescription expects a different signature (e.g., FS&), change it here
  // to match the existing pattern used by your other apps.
  static constexpr bool IsAvailable() {
    return false; // Hide the app in the UI until the screen is fully implemented.
  }
};

} // namespace Pinetime::Applications
