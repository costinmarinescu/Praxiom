#include "utility/PraxiomHealth.h"

#include <algorithm>
#include <cmath>

namespace Pinetime {
  namespace Praxiom {

    float CalculateRealtimeAdjustment(uint8_t heartRate, uint32_t steps) {
      float adjustment = 0.0f;

      if (heartRate > 0) {
        if (heartRate < 50 || heartRate > 90) {
          adjustment += 1.5f;
        } else if (heartRate >= 50 && heartRate <= 60) {
          adjustment -= 0.5f;
        } else if (heartRate >= 61 && heartRate <= 70) {
          adjustment -= 0.2f;
        } else if (heartRate >= 71 && heartRate <= 80) {
          adjustment += 0.3f;
        } else {
          adjustment += 0.8f;
        }
      }

      if (steps >= 10000) {
        adjustment -= 0.8f;
      } else if (steps >= 8000) {
        adjustment -= 0.3f;
      } else if (steps >= 5000) {
        adjustment += 0.2f;
      } else if (steps >= 3000) {
        adjustment += 0.5f;
      } else {
        adjustment += 1.0f;
      }

      return std::clamp(adjustment, -2.0f, 3.0f);
    }

    uint16_t ComputeAdjustedBioAge(uint16_t baseAgeTenths, uint8_t heartRate, uint32_t steps) {
      if (baseAgeTenths == 0) {
        baseAgeTenths = 530; // fall back to sensible default (53.0 years)
      }

      const float baseAge = static_cast<float>(baseAgeTenths) / 10.0f;
      const float finalAge = std::clamp(baseAge + CalculateRealtimeAdjustment(heartRate, steps), 18.0f, 120.0f);

      return static_cast<uint16_t>(std::round(finalAge * 10.0f));
    }
  }
}
