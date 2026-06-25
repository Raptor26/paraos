/// @file paraos_critical.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_CRITICAL_HPP
#define PARAOS_CRITICAL_HPP

#include <type_traits>
#include <variant>

#include "FreeRTOS.h"
#include "paraos_attr.h"
#include "paraos_config.hpp"
#include "task.h"

namespace paraos {

/// @brief Realization of the critical section in freeRTOS.
template <bool CAN_ISR = true>
class critical_section final {
 public:
  /// @brief Constructor ensures automatic critical section entry.
  ///
  /// @param is_isr
  explicit PARAOS_INLINE_CRITICAL critical_section(bool is_isr) noexcept
      : is_isr_{is_isr}, uxSavedInterruptStatus{0} {
    if (!is_isr_) {
      taskENTER_CRITICAL();
    } else {
      uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    }
  }

  /// @brief Reduce checking «if» condition if not using «is_isr» flag.
  explicit PARAOS_INLINE_CRITICAL critical_section() noexcept : is_isr_{false} {
    taskENTER_CRITICAL();
  }

  explicit PARAOS_INLINE_CRITICAL critical_section(std::false_type)
      : critical_section() {}

  /// @brief Destructor ensures automatic leaving of the critical section.
  ~critical_section() {
    if (!is_isr_) {
      taskEXIT_CRITICAL();
    } else {
      taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
    }
  }

  static PARAOS_INLINE_CRITICAL void force_enter() noexcept {
    taskENTER_CRITICAL();
  }

  static PARAOS_INLINE_CRITICAL void force_enter(bool is_isr) noexcept {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    force_enter();
  }

  static PARAOS_INLINE_CRITICAL void force_exit() noexcept {
    taskEXIT_CRITICAL();
  }

  static PARAOS_INLINE_CRITICAL void force_exit(bool is_isr) noexcept {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    force_exit();
  }

  PARAOS_DEPRECATED("use force_enter()")
  static PARAOS_INLINE_CRITICAL void ForceEnter(bool is_isr = false) noexcept {
    force_enter(is_isr);
  }

  PARAOS_DEPRECATED("use force_exit()")
  static PARAOS_INLINE_CRITICAL void ForceExit(bool is_isr = false) noexcept {
    force_exit(is_isr);
  }

  critical_section(const critical_section &other) = delete;
  critical_section(critical_section &&other) = delete;
  auto operator=(const critical_section &other) = delete;
  auto operator=(const critical_section &&other) = delete;

 private:
  const bool is_isr_;
  UBaseType_t uxSavedInterruptStatus;
};

/// @brief RAII critical section without ISR.
template <>
class critical_section<false> final {
 public:
  /// @brief Constructor ensures automatic critical section entry.
  explicit PARAOS_INLINE_CRITICAL critical_section() noexcept {
    taskENTER_CRITICAL();
  }

  /// @brief Destructor ensures automatic leaving of the critical section.
  ~critical_section() { taskEXIT_CRITICAL(); }

  static PARAOS_INLINE_CRITICAL void force_enter() noexcept {
    taskENTER_CRITICAL();
  }

  static PARAOS_INLINE_CRITICAL void force_enter(bool is_isr) noexcept {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    force_enter();
  }

  static PARAOS_INLINE_CRITICAL void force_exit() noexcept {
    taskEXIT_CRITICAL();
  }

  static PARAOS_INLINE_CRITICAL void force_exit(bool is_isr) noexcept {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    force_exit();
  }

  PARAOS_DEPRECATED("use force_enter()")
  static PARAOS_INLINE_CRITICAL void ForceEnter(bool is_isr = false) noexcept {
    force_enter(is_isr);
  }

  PARAOS_DEPRECATED("use force_exit()")
  static PARAOS_INLINE_CRITICAL void ForceExit(bool is_isr = false) noexcept {
    force_exit(is_isr);
  }

  critical_section(const critical_section &other) = delete;
  critical_section(critical_section &&other) = delete;
  auto operator=(const critical_section &other) = delete;
  auto operator=(const critical_section &&other) = delete;
};

template <bool CAN_ISR = true>
using CriticalSection PARAOS_DEPRECATED("use paraos::critical_section") =
    critical_section<CAN_ISR>;

inline void disable_isr() noexcept { critical_section<false>::force_enter(); }
inline void enable_isr() noexcept { critical_section<false>::force_exit(); }

PARAOS_DEPRECATED("use disable_isr()") inline void DisableIsr() noexcept {
  disable_isr();
}
PARAOS_DEPRECATED("use enable_isr()") inline void EnableIsr() noexcept {
  enable_isr();
}

}  // namespace paraos

#endif /* PARAOS_CRITICAL_HPP */
