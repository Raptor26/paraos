/// @file paraos_critical.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef CRITICAL_HPP
#define CRITICAL_HPP

#include "paraos_attr.h"
#include "paraos_recursive_mutex_std.hpp"

namespace paraos {

/// @brief Critical section implementation for Unix-like platforms.
///
/// Uses a static paraos::recursive_mutex so nested critical sections are
/// safe (for example, trace macros invoked inside atomic operations).
template <bool CAN_ISR = true>
class CriticalSection final {
 public:
  /// @brief Constructor ensures automatic critical section entry.
  ///
  /// @param is_isr Unused on Unix-like platforms (kept for API compatibility
  /// with FreeRTOS).
  explicit CriticalSection(bool is_isr = false) : is_isr_{is_isr} {
    PARAOS_ATTR_UNUSED_VAR(is_isr_);
    mutex_.lock();
  }

  /// @brief Destructor ensures automatic leaving of the critical section.
  ~CriticalSection() { mutex_.unlock(); }

  /// @brief Method is used for force disabling ISRs.
  ///
  /// @param[in] is_isr: Unused on Unix-like platforms (kept for API
  /// compatibility).
  ///
  /// @note This method is used inside ETL library macros.
  static void ForceEnter(bool is_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    mutex_.lock();
  }

  /// @brief Method is used for force enabling ISRs.
  ///
  /// @param[in] is_isr: Unused on Unix-like platforms (kept for API
  /// compatibility).
  ///
  /// @note This method is used inside ETL library macros.
  static void ForceExit(bool is_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    mutex_.unlock();
  }

  /// @brief Five rule.
  CriticalSection(CriticalSection &&other) = delete;
  auto operator=(CriticalSection &&other) -> CriticalSection & = delete;
  auto operator=(const CriticalSection &other) -> CriticalSection & = delete;
  CriticalSection(const CriticalSection &other) = delete;

 private:
  const bool is_isr_;
  static inline paraos::recursive_mutex mutex_;
};

inline void DisableIsr() { CriticalSection<false>::ForceEnter(); }
inline void EnableIsr() { CriticalSection<false>::ForceExit(); }

}  // namespace paraos

#endif /* CRITICAL_HPP */
