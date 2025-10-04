#pragma once

#include "displayapp/screens/Screen.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "Symbols.h"

#include <cstdint>

namespace Pinetime::Applications::Screens {

  class Calculator : public Screen {
  public:
    enum class Operation : uint8_t {
      None = 0,
      Add,
      Subtract,
      Multiply,
      Divide,
    };

    explicit Calculator(AppControllers& controllers);
    ~Calculator() override = default;

    void OnTouchEvent(Pinetime::Applications::TouchEvents event) override;
    void OnButtonEvent(lv_event_t event) override;

  private:
    // UI
    lv_obj_t* buttonMatrix {nullptr};
    lv_obj_t* valueLabel {nullptr};
    lv_obj_t* resultLabel {nullptr};

    // State
    Operation currentOp {Operation::None};
    bool hasDecimal {false};
    uint8_t decimals {0};
    bool negative {false};

    // Values
    int64_t value {0};
    int64_t result {0};

    // Helpers
    static constexpr int64_t powi(int64_t base, uint8_t exponent) {
      int64_t v = 1;
      while (exponent) {
        v *= base;
        --exponent;
      }
      return v;
    }

    void Eval();
    void ResetInput();
    void HandleInput();
    void UpdateValueLabel();
    void UpdateResultLabel() const;
    void UpdateOperation() const;

    // change this if you want to change the number of decimals
    // keep in mind, that we only have 12 digits available in total
    static constexpr uint8_t MaxDecimals = 4;
  };

}  // namespace Pinetime::Applications::Screens

namespace Pinetime::Applications {

  template <> struct AppTraits<Apps::Calculator> {
    static constexpr Apps app = Apps::Calculator;
    // IMPORTANT: AppDescription expects a const char* icon
    static constexpr const char* icon = Screens::Symbols::calculator;

    // IMPORTANT: The Create signature must match UserApps.h / AppDescription
    static Screens::Screens::Screen* Create(AppControllers& controllers) {
      return new Screens::Calculator(controllers);
    }

    // IMPORTANT: AppDescription expects IsAvailable(const FS&) — note the const
    static bool IsAvailable(const Pinetime::Controllers::FS& /*fs*/) {
      // Calculator is always available
      return true;
    }
  };

}  // namespace Pinetime::Applications
