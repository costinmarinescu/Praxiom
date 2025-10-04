#include <cmath>
#include "Calculator.h"
#include "displayapp/InfiniTimeTheme.h"
#include "Symbols.h"

using namespace Pinetime::Applications::Screens;

static void eventHandler(lv_obj_t* obj, lv_event_t event) {
  auto app = static_cast<Calculator*>(obj->user_data);
  app->OnButtonEvent(obj, event);
}

Calculator::~Calculator() {
  lv_obj_clean(lv_scr_act());
}

constexpr const char* const buttonMap[] = {
  "7", "8", "9", Symbols::backspace, "\n",
  "4", "5", "6", "+ -", "\n",
  "1", "2", "3", "* /", "\n",
  "0", ".", "(-)", "=", ""
};

Calculator::Calculator() {
  resultLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_long_mode(resultLabel, LV_LABEL_LONG_CROP);
  lv_label_set_align(resultLabel, LV_LABEL_ALIGN_RIGHT);
  lv_label_set_text_fmt(resultLabel, "%lld", static_cast<long long>(result));
  lv_obj_set_size(resultLabel, 200, 20);
  lv_obj_set_pos(resultLabel, 10, 5);

  valueLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_long_mode(valueLabel, LV_LABEL_LONG_CROP);
  lv_label_set_align(valueLabel, LV_LABEL_ALIGN_RIGHT);
  lv_label_set_text_fmt(valueLabel, "%lld", static_cast<long long>(value));
  lv_obj_set_size(valueLabel, 200, 20);
  lv_obj_set_pos(valueLabel, 10, 35);

  buttonMatrix = lv_btnmatrix_create(lv_scr_act(), nullptr);
  buttonMatrix->user_data = this;
  lv_obj_set_event_cb(buttonMatrix, eventHandler);
  lv_btnmatrix_set_map(buttonMatrix, const_cast<const char**>(buttonMap));
  lv_btnmatrix_set_one_check(buttonMatrix, true);
  lv_obj_set_size(buttonMatrix, 238, 180);
  lv_obj_set_style_local_bg_color(buttonMatrix, LV_BTNMATRIX_PART_BTN, LV_STATE_DEFAULT, Colors::bgAlt);
  lv_obj_set_style_local_pad_inner(buttonMatrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_pad_top(buttonMatrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_pad_bottom(buttonMatrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_pad_left(buttonMatrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_pad_right(buttonMatrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 1);
  lv_obj_align(buttonMatrix, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

  lv_obj_set_style_local_bg_opa(buttonMatrix, LV_BTNMATRIX_PART_BTN, LV_STATE_CHECKED, LV_OPA_COVER);
  lv_obj_set_style_local_bg_grad_stop(buttonMatrix, LV_BTNMATRIX_PART_BTN, LV_STATE_CHECKED, 128);
  lv_obj_set_style_local_bg_main_stop(buttonMatrix, LV_BTNMATRIX_PART_BTN, LV_STATE_CHECKED, 128);
}

void Calculator::OnButtonEvent(lv_obj_t* obj, lv_event_t event) {
  if ((obj == buttonMatrix) && (event == LV_EVENT_PRESSED)) {
    HandleInput();
  }
}

void Calculator::HandleInput() {
  const char* buttonText = lv_btnmatrix_get_active_btn_text(buttonMatrix);

  if (buttonText == nullptr) {
    return;
  }

  if ((equalSignPressedBefore && (*buttonText != '=')) || (error != Error::None)) {
    ResetInput();
    UpdateOperation();
  }

  // we only compare the first char because it is enough
  switch (*buttonText) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      // *buttonText is the first char in buttonText
      // "- '0'" results in the int value of the char
      uint8_t digit = (*buttonText) - '0';
      int8_t sign = (value < 0) ? -1 : 1;

      // if this is true, we already pressed the . button
      if (offset < FIXED_POINT_OFFSET) {
        value += sign * offset * digit;
        offset /= 10;
      } else if (value <= MAX_VALUE / 10) {
        value *= 10;
        value += sign * offset * digit;
      }
    } break;

    // unary minus
    case '(':
      value = -value;
      break;

    case '.':
      if (offset == FIXED_POINT_OFFSET) {
        offset /= 10;
      }
      break;

    // for every operator we:
