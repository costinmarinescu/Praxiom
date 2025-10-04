
#include "displayapp/screens/Calculator.h"
#include "displayapp/InfiniTimeTheme.h"
#include "Symbols.h"

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <algorithm>

/*
 * This implementation is LVGL v7-friendly.
 * If you ever move to LVGL v8+, consider replacing the btnm* calls
 * with btnmatrix* equivalents (see notes in comments).
 */

using Pinetime::Applications::Screens::Calculator;

// ====== Compile-time configuration ======
static constexpr int64_t FP = 100000;        // fixed-point scale (5 decimal places)
static constexpr int      FP_DECIMALS = 5;   // how many decimals we support
static constexpr int64_t  MAX_FP = (INT64_MAX / 4); // conservative, to avoid overflow in ops

// ====== Helpers (formatting) ======
static void format_fixed_to_buf(char* out, size_t out_sz, int64_t v_fp) {
  // Convert fixed-point to string without using floating point.
  // Shows up to FP_DECIMALS decimals, trims trailing zeros.
  bool neg = v_fp < 0;
  uint64_t av = static_cast<uint64_t>(neg ? -v_fp : v_fp);

  uint64_t int_part = av / FP;
  uint64_t frac_part = av % FP;

  // Build fractional string with leading zeros to FP_DECIMALS
  char frac[16] = {0};
  std::snprintf(frac, sizeof(frac), "%0*llu", FP_DECIMALS, (unsigned long long)frac_part);

  // Trim trailing zeros
  int frac_len = FP_DECIMALS;
  while (frac_len > 0 && frac[frac_len - 1] == '0') frac[--frac_len] = '\0';

  if (frac_len == 0) {
    std::snprintf(out, out_sz, "%s%llu", neg ? "-" : "", (unsigned long long)int_part);
  } else {
    std::snprintf(out, out_sz, "%s%llu.%s", neg ? "-" : "", (unsigned long long)int_part, frac);
  }
}

// ====== Button map (LVGL v7) ======
// NOTE: In LVGL v7, the map must end with an empty string "" and rows are separated by "\n".
static const char* btn_map[] = {
  "AC", "C",  "⌫",  "÷",  "\n",
  "7",  "8",  "9",  "×",  "\n",
  "4",  "5",  "6",  "−",  "\n",
  "1",  "2",  "3",  "+",  "\n",
  "+/-","0",  ".",  "=",  "",
};

// ====== Event thunk (LVGL v7) ======
static void btn_event_thunk(lv_obj_t* obj, lv_event_t e) {
  if (e == LV_EVENT_VALUE_CHANGED) {
    auto* self = static_cast<Calculator*>(lv_obj_get_user_data(obj));
    if (self) self->OnButtonEvent(obj, e);
  }
}

// ====== Lifecycle ======
Calculator::Calculator() {
  // Root is the current screen
  auto* parent = lv_scr_act();

  // Result (previous / running) value
  resultLabel = lv_label_create(parent, nullptr);
  lv_obj_set_width(resultLabel, LV_HOR_RES);
  lv_label_set_align(resultLabel, LV_LABEL_ALIGN_RIGHT);
  lv_label_set_text(resultLabel, "0");

  // Current input value
  valueLabel = lv_label_create(parent, nullptr);
  lv_obj_set_width(valueLabel, LV_HOR_RES);
  lv_label_set_align(valueLabel, LV_LABEL_ALIGN_RIGHT);
  lv_label_set_text(valueLabel, "0");

  // Position labels
  lv_obj_align(resultLabel, nullptr, LV_ALIGN_IN_TOP_RIGHT, -8, 6);
  lv_obj_align(valueLabel,  resultLabel, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 6);

  // Button matrix
  buttonMatrix = lv_btnm_create(parent, nullptr);
  lv_btnm_set_map(buttonMatrix, btn_map);
  lv_obj_set_width(buttonMatrix, LV_HOR_RES - 8);
  lv_obj_set_height(buttonMatrix, LV_VER_RES - 100);
  lv_obj_align(buttonMatrix, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, -6);

  // Hook events
  lv_obj_set_user_data(buttonMatrix, this);
  lv_obj_set_event_cb(buttonMatrix, btn_event_thunk);

  // Engine state
  ResetAll();
  UpdateResultLabel();
  UpdateValueLabel();
}

Calculator::~Calculator() {
  // Nothing special (labels and btnm belong to screen and will be cleaned by LVGL).
}

// ====== Public LVGL event ======
void Calculator::OnButtonEvent(lv_obj_t* obj, lv_event_t) {
  // Which button?
  uint16_t id = lv_btnm_get_active_btn(obj);
  const char* txt = lv_btnm_get_btn_text(obj, id);
  if (!txt || !*txt) return;

  HandleKey(txt);
}

// ====== Internal helpers ======
void Calculator::ResetAll() {
  value = 0;
  result = 0;
  pendingOp = 0;
  hasDecimal = false;
  decimalsEntered = 0;
  error = Error::None;
}

void Calculator::ResetInput() {
  value = 0;
  hasDecimal = false;
  decimalsEntered = 0;
}

static inline bool is_digit(const char* s) {
  return s && s[1] == '\0' && s[0] >= '0' && s[0] <= '9';
}

void Calculator::HandleKey(const char* key) {
  if (error != Error::None) {
    // Any key except AC resets from error
    if (std::strcmp(key, "AC") != 0) {
      ResetAll();
      UpdateResultLabel();
      UpdateValueLabel();
    }
  }

  if (std::strcmp(key, "AC") == 0) {
    ResetAll();
  } else if (std::strcmp(key, "C") == 0) {
    ResetInput();
  } else if (std::strcmp(key, "⌫") == 0) {
    Backspace();
  } else if (std::strcmp(key, "+/-") == 0) {
    if (value != 0) value = -value;
  } else if (std::strcmp(key, ".") == 0) {
    hasDecimal = true;
  } else if (std::strcmp(key, "+") == 0 ||
             std::strcmp(key, "−") == 0 ||
             std::strcmp(key, "×") == 0 ||
             std::strcmp(key, "÷") == 0) {
    ApplyPending();              // result (op) value
    pendingOp = key[0];          // store new op (+, −, ×, ÷ first byte)
    ResetInput();
  } else if (std::strcmp(key, "=") == 0) {
    ApplyPending();
    pendingOp = 0;
  } else if (is_digit(key)) {
    AppendDigit(key[0] - '0');
  } else {
    // ignore unknown keys
  }

  UpdateResultLabel();
  UpdateValueLabel();
}

void Calculator::Backspace() {
  if (!hasDecimal) {
    value /= 10; // drop last integer digit
  } else {
    if (decimalsEntered > 0) {
      // remove one decimal digit
      // e.g. 12.34 -> 12.3 -> 12.
      // representation: value is fixed-point scaled by FP
      // So just "un-append" last decimal digit:
      // last decimal digit was appended as d * (FP / 10^{k})
      // Equivalent here: multiply by 10 and divide by 10 removing residual
      // Simpler: convert to string and strip? But keep it pure integer math:
      // We'll track by recomputing: multiply by 10 then truncate.
      // Actually easiest (and safe): zero-out one least significant decimal place.
      int64_t sign = value < 0 ? -1 : 1;
      int64_t av = std::llabs(value);
      av = (av / 10); // drop one decimal place in fixed-point domain
      value = sign * av;
      decimalsEntered--;
      if (decimalsEntered == 0) hasDecimal = false;
    }
  }
}

void Calculator::AppendDigit(uint8_t d) {
  if (!hasDecimal) {
    // value = value * 10 + d, guarding overflow
    if (std::llabs(value) > (MAX_FP - d) / 10) {
      error = Error::TooLarge;
      return;
    }
    value = value * 10 + (value >= 0 ? d : -d);
  } else {
    if (decimalsEntered >= FP_DECIMALS) return; // ignore extra decimals
    // append fractional digit: value += sign * (FP / 10^{k+1}) * d
    int64_t unit = FP;
    for (int i = 0; i <= decimalsEntered; ++i) unit /= 10;
    if (unit == 0) return;
    int64_t delta = static_cast<int64_t>(d) * unit;
    if (value < 0) delta = -delta;
    if ((value > 0 && value > MAX_FP - delta) || (value < 0 && value < -MAX_FP - delta)) {
      error = Error::TooLarge;
      return;
    }
    value += delta;
    decimalsEntered++;
  }
}

static bool will_mul_overflow(int64_t a, int64_t b) {
  if (a == 0 || b == 0) return false;
  if (a == -1) return b == INT64_MIN;
  if (b == -1) return a == INT64_MIN;
  __int128 t = static_cast<__int128>(a) * static_cast<__int128>(b);
  return t > INT64_MAX || t < INT64_MIN;
}

void Calculator::ApplyPending() {
  if (error != Error::None) return;

  // If there is no pending op, move value to result
  if (pendingOp == 0) {
    result = value;
    return;
  }

  // Compute "result (op) value"
  int64_t a = result;
  int64_t b = value;

  switch (pendingOp) {
    case '+':
      if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) {
        error = Error::TooLarge;
        return;
      }
      result = a + b;
      break;
    case '−': // U+2212 minus sign
    case '-':
      if ((b < 0 && a > INT64_MAX + b) || (b > 0 && a < INT64_MIN + b)) {
        error = Error::TooLarge;
        return;
      }
      result = a - b;
      break;
    case '×': // U+00D7 multiplication sign
    case '*':
      // (a * b) / FP
      if (will_mul_overflow(a, b)) { error = Error::TooLarge; return; }
      result = static_cast<int64_t>((static_cast<__int128>(a) * static_cast<__int128>(b)) / FP);
      break;
    case '÷': // U+00F7 division sign
    case '/':
      if (b == 0) { error = Error::ZeroDivision; return; }
      // (a * FP) / b
      if (will_mul_overflow(a, FP)) { error = Error::TooLarge; return; }
      result = static_cast<int64_t>((static_cast<__int128>(a) * FP) / b);
      break;
    default:
      break;
  }
}

void Calculator::UpdateResultLabel() const {
  char buf[48];
  if (error == Error::ZeroDivision) {
    std::snprintf(buf, sizeof(buf), "Error: ÷0");
  } else if (error == Error::TooLarge) {
    std::snprintf(buf, sizeof(buf), "Error: overflow");
  } else {
    format_fixed_to_buf(buf, sizeof(buf), result);
  }
  lv_label_set_text(resultLabel, buf);
}

void Calculator::UpdateValueLabel() {
  char buf[48];
  if (error != Error::None) {
    lv_label_set_text(valueLabel, "");
    return;
  }
  format_fixed_to_buf(buf, sizeof(buf), value);
  lv_label_set_text(valueLabel, buf);
}
