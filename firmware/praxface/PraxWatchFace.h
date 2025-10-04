#pragma once
#include "displayapp/screens/WatchFace.h"
#include "lvgl/lvgl.h"

namespace Pinetime { namespace Applications {
class PraxWatchFace : public DisplayApp::Screens::WatchFace {
 public:
  PraxWatchFace();
  ~PraxWatchFace() override;
  void Refresh() override;
  void SetBioAge(float age);
  void SetSpO2(uint8_t v);
  void SetHRV(uint16_t v);
 private:
  void BuildUi();
  void ApplyBackground();
  lv_obj_t* cont_{nullptr};
  lv_obj_t* imgBg_{nullptr};
  lv_obj_t* lblBioAge_{nullptr};
  lv_obj_t* lblTime_{nullptr};
  lv_obj_t* lblDate_{nullptr};
  lv_style_t styleLarge_;
  lv_style_t styleMedium_;
  lv_style_t styleSmall_;
  float bioAge_ = -1.0f;
};
}} // namespaces
