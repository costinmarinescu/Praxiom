#include "utility/PraxiomHealth.h"

#include <algorithm>

namespace Pinetime {
  namespace Praxiom {

    uint16_t ComputeAdjustedBioAge(uint16_t baseAgeTenths, uint8_t /*heartRate*/, uint32_t /*steps*/) {
      if (baseAgeTenths == 0) {
        baseAgeTenths = 530; // fall back to sensible default (53.0 years)
      }

      return std::clamp(baseAgeTenths, static_cast<uint16_t>(180), static_cast<uint16_t>(1200));
    }
  }
}
