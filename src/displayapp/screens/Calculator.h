#pragma once

#include "displayapp/screens/Screen.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include <cstdint> // int64_t, uint8_t

// Header-safe constexpr helper for fixed-point scale.
namespace Pinetime::Applications::Screens {
constexpr inline int64_t pow10(uint8_t n) {
  return n == 0 ? 1 : 10 * pow10(static_cast<uint8_t>(n - 1));
}
} // namespace Pinetime::Applications::Screens

namespace Pinetime {
namespace Applications {

class AppControllers;

namespace Screens {

class Calculator : public Screen {
public:
  Calculator();
  ~Calculator() override;

  void OnButtonEvent(lv_obj_t* obj, lv_event_t event);

private:
  // Core state
  int64_t value                     = 0;   // input value (fixed-point)
  int64_t otherValue                = 0;   // stored value (fixed-point)
  char    operation                 = ' '; // '+', '-', '*', '/', or ' ' (none)
  bool    inputTargetIsValue        = true;
  bool    resultsAlreadyCalculated  = false;

  // UI
  lv_obj_t* operatorLabel {};
  lv_obj_t* valueLabel {};
  lv_obj_t* resultLabel {};

  // Behaviour
  void Eval();
  void ResetInput();
  void HandleInput();
  void UpdateValueLabel();
  void UpdateResultLabel() const;
  void UpdateOperation() const;

  // Fixed-point configuration
  static constexpr uint8_t MAX_DIGITS         = 12;
  static constexpr uint8_t N_DECIMALS         = 3;
  static constexpr int64_t FIXED_POINT_OFFSET = Screens::pow10(N_DECIMALS);
  static constexpr int64_t MAX_VALUE          = Screens::pow10(MAX_DIGITS) - 1;
  static constexpr int64_t MIN_VALUE          = -MAX_VALUE;
};

} // namespace Screens

// AppTraits specialization required by UserApps.h
template <>
struct AppTraits<Apps::Calculator> {
  static constexpr Apps app = Apps::Calculator;

  // Avoid a hard dependency on Symbols.* so this header compiles in all TU orders.
  // If you want to hook into your symbol set, you can change this to that value,
  // but 'C' is a safe, always-available fallback.
  static constexpr char icon = 'C';

  static Screens::Screen* Create(AppControllers& /*controllers*/) {
    return new Screens::Calculator();
  }

  // UserApps.h expects this function pointer (&AppTraits<t>::IsAvailable).
  static constexpr bool IsAvailable() { return true; }
};

} // namespace Applications
} // namespace Pinetime
