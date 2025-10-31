#pragma once

#include <cstdint>

namespace Pinetime {
  namespace Praxiom {
    uint16_t ComputeAdjustedBioAge(uint16_t baseAgeTenths, uint8_t heartRate, uint32_t steps);
  }
}
