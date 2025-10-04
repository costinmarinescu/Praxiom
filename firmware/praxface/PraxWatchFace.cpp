#include "PraxWatchFace.h"
#include "systemtask/SystemTask.h"
#include "praxsettings/PraxSettingsStore.h"
#include <cstdio>

extern const lv_img_dsc_t dna_bg_light;
extern const lv_img_dsc_t dna_bg_dark;

using namespace Pinetime::Applications;

PraxWatchFace::PraxWatchFace(){ BuildUi(); }
PraxWatchFace::~PraxWatchFace(){}

void PraxWatchFace::BuildUi(){
  cont_ = lv_obj_create(lv_scr_act());
  lv_obj_remove_style_all(cont_);
  lv_obj_set_size(cont_, 240, 240);
  lv_obj_center(cont_);

  imgBg_ = lv_img_create(cont_);
  ApplyBackground();
  lv_obj_center(imgBg_);

  lv_style_init(&styleLarge_);
  lv_style_set_text_color(&styleLarge_, lv_color_white());
  lv_style_set_text_font(&styleLarge_, &lv_font_montserrat_46);

  lv_style_init(&styleMedium_);
  lv_style_set_text_color(&styleMedium_, lv_color_white());
  lv_style_set_text_font(&styleMedium_, &lv_font_montserrat_26);

  lv_style_init(&styleSmall_);
  lv_style_set_text_color(&styleSmall_, lv_color_white());
  lv_style_set_text_font(&styleSmall_, &lv_font_montserrat_16);

  lblBioAge_ = lv_label_create(cont_);
  lv_obj_add_style(lblBioAge_, &styleLarge_, 0);
  lv_label_set_text(lblBioAge_, "--");
  lv_obj_align(lblBioAge_, LV_ALIGN_CENTER, 0, -18);

  lblTime_ = lv_label_create(cont_);
  lv_obj_add_style(lblTime_, &styleMedium_, 0);
  lv_label_set_text(lblTime_, "00:00");
  lv_obj_align(lblTime_, LV_ALIGN_CENTER, 0, 24);

  lblDate_ = lv_label_create(cont_);
  lv_obj_add_style(lblDate_, &styleSmall_, 0);
  lv_label_set_text(lblDate_, "Sat 04");
  lv_obj_align(lblDate_, LV_ALIGN_BOTTOM_MID, 0, -8);
}

void PraxWatchFace::ApplyBackground(){
  bool dark = PraxSettingsStore::Instance().IsDarkMotif();
  lv_img_set_src(imgBg_, dark ? &dna_bg_dark : &dna_bg_light);
}

void PraxWatchFace::SetBioAge(float a){
  bioAge_ = a;
  char buf[16];
  if(a < 0) snprintf(buf, sizeof(buf), "--");
  else snprintf(buf, sizeof(buf), "%.0f", a);
  lv_label_set_text(lblBioAge_, buf);
}

void PraxWatchFace::SetSpO2(uint8_t v){ (void)v; }
void PraxWatchFace::SetHRV(uint16_t v){ (void)v; }

void PraxWatchFace::Refresh(){
  auto now = Pinetime::Controllers::DateTime::Now();
  char tbuf[6]; snprintf(tbuf, sizeof(tbuf), "%02u:%02u", now.hours, now.minutes);
  lv_label_set_text(lblTime_, tbuf);

  char dbuf[16]; static const char* wd[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  snprintf(dbuf, sizeof(dbuf), "%s %02u", wd[now.dayOfWeek], now.day);
  lv_label_set_text(lblDate_, dbuf);
  ApplyBackground();
}
