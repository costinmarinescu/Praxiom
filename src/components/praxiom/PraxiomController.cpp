#include "components/praxiom/PraxiomController.h"

using namespace Pinetime::Controllers;

void PraxiomController::SetBioAge(float age) {
  if (age > 0.0f && age < 200.0f) {
    bioAge = age;
    bioAgeReceived = true;
  }
}

float PraxiomController::GetBioAge() const {
  return bioAge;
}

bool PraxiomController::HasBioAge() const {
  return bioAgeReceived;
}

void PraxiomController::Reset() {
  bioAge = 0.0f;
  bioAgeReceived = false;
}
