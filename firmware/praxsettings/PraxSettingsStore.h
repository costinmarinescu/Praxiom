#pragma once
#include <cstdint>
class PraxSettingsStore {
public:
  static PraxSettingsStore& Instance();
  bool IsDarkMotif() const { return darkMotif_; }
  uint8_t HrvAvgWindow() const { return hrvAvgWindow_; } // 0=30s,1=1m,2=5m
  uint8_t Spo2Floor() const { return spo2Floor_; } // 92..95
  uint8_t BioAgeSource() const { return bioAgeSource_; } // 0 wearable, 1 lab+wearable
  void SetDarkMotif(bool v); void SetHrvAvgWindow(uint8_t v);
  void SetSpo2Floor(uint8_t v); void SetBioAgeSource(uint8_t v);
  void ResetDefaults();
private:
  PraxSettingsStore() = default;
  bool darkMotif_ = false; uint8_t hrvAvgWindow_ = 1; uint8_t spo2Floor_ = 93; uint8_t bioAgeSource_ = 1;
};
