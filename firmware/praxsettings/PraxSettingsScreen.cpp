#include "PraxSettingsScreen.h"
#include "PraxSettingsStore.h"

PraxSettingsScreen::PraxSettingsScreen(){
  list_ = lv_list_create(lv_scr_act());
  auto b1 = lv_list_add_btn(list_, LV_SYMBOL_IMAGE, "Dark DNA motif");
  lv_obj_add_event_cb(b1, [](lv_event_t* e){ auto &s=PraxSettingsStore::Instance(); s.SetDarkMotif(!s.IsDarkMotif()); }, LV_EVENT_CLICKED, nullptr);
  auto b2 = lv_list_add_btn(list_, LV_SYMBOL_SETTINGS, "HRV window (30s/1m/5m)");
  lv_obj_add_event_cb(b2, [](lv_event_t* e){ auto &s=PraxSettingsStore::Instance(); s.SetHrvAvgWindow((s.HrvAvgWindow()+1)%3); }, LV_EVENT_CLICKED, nullptr);
  auto b3 = lv_list_add_btn(list_, LV_SYMBOL_PLUS, "SpO2 floor +1");
  lv_obj_add_event_cb(b3, [](lv_event_t* e){ auto &s=PraxSettingsStore::Instance(); auto v=s.Spo2Floor(); if(v<95) s.SetSpo2Floor(v+1); }, LV_EVENT_CLICKED, nullptr);
  auto b4 = lv_list_add_btn(list_, LV_SYMBOL_SHUFFLE, "Bio-Age source (cycle)");
  lv_obj_add_event_cb(b4, [](lv_event_t* e){ auto &s=PraxSettingsStore::Instance(); s.SetBioAgeSource((s.BioAgeSource()+1)%2); }, LV_EVENT_CLICKED, nullptr);
  auto b5 = lv_list_add_btn(list_, LV_SYMBOL_REFRESH, "Reset defaults");
  lv_obj_add_event_cb(b5, [](lv_event_t* e){ PraxSettingsStore::Instance().ResetDefaults(); }, LV_EVENT_CLICKED, nullptr);
}
PraxSettingsScreen::~PraxSettingsScreen(){}
