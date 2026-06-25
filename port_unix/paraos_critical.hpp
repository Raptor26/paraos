/// @file paraos_critical.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_CRITICAL_HPP
#define PARAOS_CRITICAL_HPP

#include "paraos_attr.h"
#include "paraos_config.hpp"
#include "paraos_recursive_mutex_std.hpp"

namespace paraos {

/// @brief Critical section implementation for Unix-like platforms.
///
/// Uses a static paraos::recursive_mutex so nested critical sections are
/// safe (for example, trace macros invoked inside atomic operations).
template <bool CAN_ISR = true>
class critical_section final {
 public:
  /// @brief Constructor ensures automatic critical section entry.
  ///
  /// @param is_isr Unused on Unix-like platforms (kept for API compatibility
  /// with FreeRTOS).
  explicit critical_section(bool is_isr = false) : is_isr_{is_isr} {
    PARAOS_ATTR_UNUSED_VAR(is_isr_);
    mutex_.lock();
  }

  /// @brief Destructor ensures automatic leaving of the critical section.
  ~critical_section() { mutex_.unlock(); }

  /// @brief Method is used for force disabling ISRs.
  ///
  /// @param[in] is_isr: Unused on Unix-like platforms (kept for API
  /// compatibility).
  ///
  /// @note This method is used inside ETL library macros.
  static void force_enter(bool is_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    mutex_.lock();
  }

  /// @brief Method is used for force enabling ISRs.
  ///
  /// @param[in] is_isr: Unused on Unix-like platforms (kept for API
  /// compatibility).
  ///
  /// @note This method is used inside ETL library macros.
  static void force_exit(bool is_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    mutex_.unlock();
  }

  PARAOS_DEPRECATED("use force_enter()")
  static void ForceEnter(bool is_isr = false) {
    force_enter(is_isr);
  }

  PARAOS_DEPRECATED("use force_exit()")
  static void ForceExit(bool is_isr = false) {
    force_exit(is_isr);
  }

  /// @brief Five rule.
  critical_section(critical_section &&other) = delete;
  auto operator=(critical_section &&other) -> critical_section & = delete;
  auto operator=(const critical_section &other) -> critical_section & = delete;
  critical_section(const critical_section &other) = delete;

 private:
  const bool is_isr_;
  static inline paraos::recursive_mutex mutex_;
};

template <bool CAN_ISR = true>
using CriticalSection PARAOS_DEPRECATED("use paraos::critical_section") =
    critical_section<CAN_ISR>;

inline void disable_isr() { critical_section<false>::force_enter(); }
inline void enable_isr() { critical_section<false>::force_exit(); }

PARAOS_DEPRECATED("use disable_isr()") inline void DisableIsr() {
  disable_isr();
}
PARAOS_DEPRECATED("use enable_isr()") inline void EnableIsr() {
  enable_isr();
}

}  // namespace paraos

#endif /* PARAOS_CRITICAL_HPP */
