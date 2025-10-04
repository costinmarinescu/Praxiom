#pragma once

#include "displayapp/screens/Screen.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "Symbols.h"
#include <cstdint>  // for int64_t, uint8_t

// Put small constexpr helpers in the owning namespace (header-safe).
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
        ~Calculator() override;                          // keep out-of-line (defined in .cpp)

        void OnButtonEvent(lv_obj_t* obj, lv_event_t event);

      private:
        // Core state
        int64_t value                   = 0;             // input value in fixed-point
        int64_t otherValue              = 0;             // stored value in fixed-point
        char     operation              = ' ';           // '+', '-', '*', '/', or ' ' (none)
        bool     inputTargetIsValue     = true;          // true: editing 'value', false: 'otherValue'
        bool     resultsAlreadyCalculated = false;

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

        // Fixed-point configuration:
        // We have MAX_DIGITS total digits and N_DECIMALS fractional digits.
        static constexpr uint8_t MAX_DIGITS        = 12;
        static constexpr uint8_t N_DECIMALS        = 3;

        // Constant scale factor (e.g., N_DECIMALS=3 -> 1000)
        static constexpr int64_t FIXED_POINT_OFFSET = pow10(N_DECIMALS);

        // Clamp bounds for 64-bit fixed-point representation
        static constexpr int64_t MAX_VALUE          = pow10(MAX_DIGITS) - 1;
        static constexpr int64_t MIN_VALUE          = -MAX_VALUE;
      };

    } // namespace Screens

    template <>
    struct AppTraits<Apps::Calculator> {
      static constexpr Apps app = Apps::Calculator;
      static constexpr char icon = Symbols::calculator;
      static Screens::Screen* Create(AppControllers& /*controllers*/) {
        return new Screens::Calculator();
      }
    };

  } // namespace Applications
} // namespace Pinetime
