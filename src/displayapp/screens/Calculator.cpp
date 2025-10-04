#include "Calculator.h"

#include <cstring>
#include <cstdio>

namespace Pinetime {
namespace Applications {
namespace Screens {

// Simple overflow-safe helpers using compiler builtins when available
static inline bool add_overflow(int64_t a, int64_t b, int64_t* out) {
#if defined(__has_builtin)
#  if __has_builtin(__builtin_add_overflow)
  return __builtin_add_overflow(a, b, out);
#  else
  *out = a + b;
  // Conservative overflow check
  if ((b > 0 && *out < a) || (b < 0 && *out > a)) return true;
  return false;
#  endif
#else
  *out = a + b;
  if ((b > 0 && *out < a) || (b < 0 && *out > a)) return true;
  return false;
#endif
}

static inline bool sub_overflow(int64_t a, int64_t b, int64_t* out) {
#if defined(__has_builtin)
#  if __has_builtin(__builtin_sub_overflow)
  return __builtin_sub_overflow(a, b, out);
#  else
  *out = a - b;
  if ((b < 0 && *out < a) || (b > 0 && *out > a)) return true;
  return false;
#  endif
#else
  *out = a - b;
  if ((b < 0 && *out < a) || (b > 0 && *out > a)) return true;
  return false;
#endif
}

static inline bool mul_overflow(int64_t a, int64_t b, int64_t* out) {
#if defined(__has_builtin)
#  if __has_builtin(__builtin_mul_overflow)
  return __builtin_mul_overflow(a, b, out);
#  else
  // Fallback: very conservative
  if (a == 0 || b == 0) { *out = 0; return false; }
  if (a > 0 ? (b > INT64_MAX / a) : (b < INT64_MIN / a)) return true;
  if (a < 0 && b > 0 && a < INT64_MIN / b) return true;
  if (a > 0 && b < 0 && b < INT64_MIN / a) return true;
  *out = a * b;
  return false;
#  endif
#else
  if (a == 0 || b == 0) { *out = 0; return false; }
  if (a > 0 ? (b > INT64_MAX / a) : (b < INT64_MIN / a)) return true;
  if (a < 0 && b > 0 && a < INT64_MIN / b) return true;
  if (a > 0 && b < 0 && b < INT64_MIN / a) return true;
  *out = a * b;
  return false;
#endif
}

// Format fixed-point value (2 decimals) into a buffer
static void format_fixed_to_buf(char* out, size_t out_sz, int64_t v_fp) {
  const bool neg = v_fp < 0;
  if (neg) v_fp = -v_fp;
  const int64_t intp = v_fp / 100;
  const int64_t frac = v_fp % 100;
  if (neg) std::snprintf(out, out_sz, "-%lld.%02lld", (long long)intp, (long long)frac);
  else     std::snprintf(out, out_sz,  "%lld.%02lld", (long long)intp, (long long)frac);
}

// ---- Public API ------------------------------------------------------------

Calculator::Calculator(AppControllers& /*controllers*/) {
  // Minimal, UI creation intentionally omitted to keep this screen lightweight
  ResetAll();
}

Calculator::~Calculator() = default;

void Calculator::OnButtonEvent(lv_obj_t* /*obj*/, lv_event_t /*e*/) {
  // In the current minimal implementation, this is a no-op.
  // If you wire this up to an LVGL btnmatrix, call HandleKey() with button text here.
}

// ---- Logic -----------------------------------------------------------------

void Calculator::ResetAll() {
  acc_ = 0;
  entry_ = 0;
  op_ = '\0';
  entering_ = false;
  UpdateResultLabel();
  UpdateValueLabel();
}

void Calculator::ResetInput() {
  entry_ = 0;
  entering_ = false;
  UpdateValueLabel();
}

void Calculator::Backspace() {
  entry_ /= 10;
  UpdateValueLabel();
}

void Calculator::AppendDigit(uint8_t d) {
  if (d > 9) return;
  entering_ = true;
  // Append digit to fixed-point *100 (treat as integer entry for simplicity here)
  int64_t tmp;
  if (mul_overflow(entry_, 10, &tmp)) return; // ignore on overflow
  entry_ = tmp + (int64_t)d;
  UpdateValueLabel();
}

void Calculator::ApplyPending() {
  if (!op_) { acc_ = entry_; return; }
  int64_t out = acc_;
  switch (op_) {
    case '+':
      if (!add_overflow(acc_, entry_, &out)) acc_ = out;
      break;
    case '-':
      if (!sub_overflow(acc_, entry_, &out)) acc_ = out;
      break;
    case '*':
      if (!mul_overflow(acc_, entry_, &out)) acc_ = out;
      break;
    case '/':
      if (entry_ != 0) acc_ = acc_ / entry_;
      break;
    default:
      break;
  }
}

void Calculator::HandleKey(const char* key) {
  if (!key || !*key) return;
  if (std::strcmp(key, "C") == 0) { ResetAll(); return; }
  if (std::strcmp(key, "CE") == 0) { ResetInput(); return; }
  if (std::strcmp(key, "<") == 0) { Backspace(); return; }
  if (std::strcmp(key, "+") == 0 ||
      std::strcmp(key, "-") == 0 ||
      std::strcmp(key, "*") == 0 ||
      std::strcmp(key, "/") == 0) {
    ApplyPending();
    op_ = key[0];
    entry_ = 0;
    entering_ = false;
    UpdateResultLabel();
    UpdateValueLabel();
    return;
  }
  if (std::strcmp(key, "=") == 0) {
    ApplyPending();
    op_ = '\0';
    entry_ = acc_;
    UpdateResultLabel();
    UpdateValueLabel();
    return;
  }

  // Digits
  if (key[1] == '\0' && key[0] >= '0' && key[0] <= '9') {
    AppendDigit(static_cast<uint8_t>(key[0] - '0'));
    return;
  }
}

void Calculator::UpdateResultLabel() const {
  (void)resultLabel_; // UI not wired in minimal implementation
  // Example formatting (kept to avoid -Werror=unused-function)
  char buf[24];
  format_fixed_to_buf(buf, sizeof(buf), acc_);
  (void)buf;
}

void Calculator::UpdateValueLabel() {
  (void)valueLabel_;
  char buf[24];
  format_fixed_to_buf(buf, sizeof(buf), entry_);
  (void)buf;
}

} // namespace Screens
} // namespace Applications
} // namespace Pinetime
