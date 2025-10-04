#pragma once

#include "displayapp/screens/Screen.h"
#include "displayapp/Controllers.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {

      class Calculator : public Screen {
      public:
        explicit Calculator(AppControllers& /*controllers*/) {}
        ~Calculator() override = default;

        // Correct signature: base returns bool
        bool OnTouchEvent(TouchEvents /*event*/) override { 
          return false; 
        }

        // Hardware side button handler expected by Screen
        bool OnButtonPushed() override { 
          return false; 
        }
      };
    }
  }
}
