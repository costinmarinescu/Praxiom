#pragma once
#include "displayapp/screens/Screen.h"
#include "lvgl/lvgl.h"
class PraxSettingsScreen : public Pinetime::Applications::Screen {
public:
  PraxSettingsScreen();
  ~PraxSettingsScreen() override;
  void Refresh() override {}
private:
  lv_obj_t* list_{nullptr};
};
