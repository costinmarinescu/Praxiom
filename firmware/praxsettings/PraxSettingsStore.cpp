#include "PraxSettingsStore.h"
static PraxSettingsStore gStore;
PraxSettingsStore& PraxSettingsStore::Instance(){ return gStore; }
void PraxSettingsStore::SetDarkMotif(bool v){ darkMotif_=v; }
void PraxSettingsStore::SetHrvAvgWindow(uint8_t v){ hrvAvgWindow_=v; }
void PraxSettingsStore::SetSpo2Floor(uint8_t v){ spo2Floor_=v; }
void PraxSettingsStore::SetBioAgeSource(uint8_t v){ bioAgeSource_=v; }
void PraxSettingsStore::ResetDefaults(){ darkMotif_=false; hrvAvgWindow_=1; spo2Floor_=93; bioAgeSource_=1; }
