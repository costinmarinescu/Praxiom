#pragma once

#include <cstdint>

namespace Pinetime {
  namespace Controllers {
    class PraxiomController {
    public:
      PraxiomController() = default;
      
      void SetBioAge(float age);
      float GetBioAge() const;
      bool HasBioAge() const;
      
      void Reset();
      
    private:
      float bioAge = 0.0f;
      bool bioAgeReceived = false;
    };
  }
}
