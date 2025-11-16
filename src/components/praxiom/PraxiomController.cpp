#include "components/praxiom/PraxiomController.h"

using namespace Pinetime::Controllers;

void PraxiomController::SetBioAge(uint8_t age) {
  bioAge = age;
  ageUpdated = true;
}

uint8_t PraxiomController::GetBioAge() const {
  return bioAge;
}

void PraxiomController::SetAgeUpdated(bool updated) {
  ageUpdated = updated;
}

bool PraxiomController::IsAgeUpdated() const {
  return ageUpdated;
}
