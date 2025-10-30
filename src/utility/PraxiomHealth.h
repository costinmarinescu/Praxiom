#pragma once

#include <cstdint>

namespace Pinetime {
  namespace Praxiom {
    float CalculateRealtimeAdjustment(uint8_t heartRate, uint32_t steps);
    uint16_t ComputeAdjustedBioAge(uint16_t baseAgeTenths, uint8_t heartRate, uint32_t steps);
  }
}
