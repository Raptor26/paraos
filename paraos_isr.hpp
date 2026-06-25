/// @file paraos_isr.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PAROAS_ISR_HPP
#define PAROAS_ISR_HPP

#include "paraos_config.hpp"

namespace paraos {

/// @brief Contained bool value and additional bool, specified is need switch
/// RTOS contest.
/// @note Useful in API, which called from isr (xSemaphoreGiveFromISR() in
/// freeRTOS as exapmle). Semaphore::Give() with this class can transfer
/// information in called code, which can call method for switch context RTOS if
/// needed.
///
/// Disabling clang-tidy checks because the variable is_success_ is set using
/// operator=. This overload is used in other constructors without initial
/// initialization of the is_success_ field, which leads to warnings from the
/// static analyzer.
// NOLINTBEGIN(*-member-init)
struct isr_bool final {
  explicit isr_bool(bool is_success = false, bool is_need_switch_context = false)
      : is_success_{is_success},
        is_need_switch_context_{is_need_switch_context} {}

  ~isr_bool() = default;

  /// --------------------------------------------------------------------------
  /// Five rule
  /// --------------------------------------------------------------------------

  /// @note Don't initialize is_success_ because the field initialize with
  /// operator=.
  isr_bool(const isr_bool &other) noexcept : is_need_switch_context_{false} {
    *this = other;
  };

  /// @note Don't initialize is_success_ because the field initialize with
  /// operator=.
  isr_bool(isr_bool &&other) noexcept : is_need_switch_context_{false} {
    *this = other;
  };

  auto operator=(const isr_bool &other) noexcept -> isr_bool & {
    if (&other != this) {
      is_success_ = other.is_success_;

      if (other.is_need_switch_context_) {
        is_need_switch_context_ = other.is_need_switch_context_;
      }
    }

    return *this;
  };

  auto operator=(isr_bool &&other) noexcept -> isr_bool & {
    *this = other;

    return *this;
  };

  /// @brief  Behavior like as simple bool variable.
  explicit operator bool() const { return is_success_; }

  [[nodiscard]] auto needs_context_switch() const -> bool {
    return is_need_switch_context_;
  }

  void set_success_status(bool status) { is_success_ = status; }

  void set_switch_context_status(bool status) { is_need_switch_context_ = status; }

  // Backward-compatible deprecated forwarding methods.
  [[nodiscard]] PARAOS_DEPRECATED("use needs_context_switch()")
  auto IsNeedSwitchContext() const -> bool {
    return needs_context_switch();
  }

  PARAOS_DEPRECATED("use set_success_status()")
  void SetSuccessStatus(bool status) { set_success_status(status); }

  PARAOS_DEPRECATED("use set_switch_context_status()")
  void SetSwitchContextStatus(bool status) { set_switch_context_status(status); }

 private:
  /// @brief Bool flag. This value returned 'operator bool()', just like simple
  /// bool variable.
  bool is_success_;

  /// @brief Is true, we need switch scheduler context. Useful when
  /// semaphore/mutex api called from ISR.
  bool is_need_switch_context_;
};
// NOLINTEND(*-member-init)

using ISRbool PARAOS_DEPRECATED("use paraos::isr_bool") = isr_bool;

}  // namespace paraos

#endif /* PAROAS_ISR_HPP */
