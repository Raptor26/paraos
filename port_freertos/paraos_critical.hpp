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
class CriticalSection final {
 public:
  /// @brief Constructor ensures automatic critical section entry.
  ///
  /// @param is_isr
  explicit PARAOS_INLINE_CRITICAL CriticalSection(bool is_isr) noexcept
      : is_isr_{is_isr}, uxSavedInterruptStatus{0} {
    if (!is_isr_) {
      taskENTER_CRITICAL();
    } else {
      uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    }
  }

  /// @brief Reduce checking «if» condition if not using «is_isr» flag.
  explicit PARAOS_INLINE_CRITICAL CriticalSection() noexcept : is_isr_{false} {
    taskENTER_CRITICAL();
  }

  explicit PARAOS_INLINE_CRITICAL CriticalSection(std::false_type)
      : CriticalSection() {}

  /// @brief Destructor ensures automatic leaving of the critical section.
  ~CriticalSection() {
    if (!is_isr_) {
      taskEXIT_CRITICAL();
    } else {
      taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
    }
  }

  CriticalSection(const CriticalSection &other) = delete;
  CriticalSection(CriticalSection &&other) = delete;
  auto operator=(const CriticalSection &other) = delete;
  auto operator=(const CriticalSection &&other) = delete;

 private:
  const bool is_isr_;
  UBaseType_t uxSavedInterruptStatus;
};

/// @brief RAII critical section without ISR.
template <>
class CriticalSection<false> final {
 public:
  /// @brief Constructor ensures automatic critical section entry.
  explicit PARAOS_INLINE_CRITICAL CriticalSection() noexcept {
    taskENTER_CRITICAL();
  }

  /// @brief Destructor ensures automatic leaving of the critical section.
  ~CriticalSection() { taskEXIT_CRITICAL(); }

  CriticalSection(const CriticalSection &other) = delete;
  CriticalSection(CriticalSection &&other) = delete;
  auto operator=(const CriticalSection &other) = delete;
  auto operator=(const CriticalSection &&other) = delete;
};

inline void DisableIsr() noexcept { taskENTER_CRITICAL(); }
inline void EnableIsr() noexcept { taskEXIT_CRITICAL(); }
}  // namespace paraos

#endif /* PARAOS_CRITICAL_HPP */
