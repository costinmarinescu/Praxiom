#pragma once

#include <cstdint>

namespace Pinetime {
  namespace Controllers {
    class PraxiomController {
    public:
      void SetBioAge(uint8_t age);
      uint8_t GetBioAge() const;
      
      void SetAgeUpdated(bool updated);
      bool IsAgeUpdated() const;

    private:
      uint8_t bioAge = 0;
      bool ageUpdated = false;
    };
  }
}
