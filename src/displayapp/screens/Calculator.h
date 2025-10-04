#pragma once

#include "Screen.h"
#include <cstdint>

// Forward declarations to avoid heavy includes
struct _lv_obj_t;
typedef _lv_obj_t lv_obj_t;

// LVGL event type forward declaration (v7 signature used in this project)
typedef enum {
  LV_EVENT_PRESSED,
  LV_EVENT_SHORT_CLICKED,
  LV_EVENT_CLICKED,
  LV_EVENT_LONG_PRESSED,
  LV_EVENT_LONG_PRESSED_REPEAT,
  LV_EVENT_VALUE_CHANGED,
  LV_EVENT_RELEASED,
  LV_EVENT_DRAG_BEGIN,
  LV_EVENT_DRAG_END,
  LV_EVENT_DRAG_THROW_BEGIN,
  LV_EVENT_GESTURE,
  LV_EVENT_KEY,
  LV_EVENT_FOCUSED,
  LV_EVENT_DEFOCUSED,
  LV_EVENT_LEAVE,
  LV_EVENT_READY,
  LV_EVENT_CANCEL
} lv_event_t;

namespace Pinetime {
namespace Applications {

class AppControllers;

namespace Screens {

class Calculator : public Screen {
public:
  explicit Calculator(AppControllers& controllers);
  ~Calculator() override;

  // Event entrypoint used by the LVGL thunk
  void OnButtonEvent(lv_obj_t* obj, lv_event_t e);

private:
  void ResetAll();
  void ResetInput();
  void HandleKey(const char* key);
  void Backspace();
  void AppendDigit(uint8_t d);
  void ApplyPending();
  void UpdateResultLabel() const;
  void UpdateValueLabel();

  // UI widgets (created in ctor if LVGL is available at runtime)
  lv_obj_t* container_ = nullptr;
  lv_obj_t* valueLabel_ = nullptr;
  lv_obj_t* resultLabel_ = nullptr;
  lv_obj_t* buttonMatrix_ = nullptr;

  // Calculator state (fixed-point integer arithmetic: value * 100 for 2 decimals)
  int64_t acc_ = 0;      // accumulated result in fixed-point
  int64_t entry_ = 0;    // current entry in fixed-point
  char op_ = '\0';       // pending op: '+', '-', '*', '/', or 0
  bool entering_ = false;
};

} // namespace Screens
} // namespace Applications
} // namespace Pinetime
