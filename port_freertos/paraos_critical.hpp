/// @file paraos_critical.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author VyhodcevEgor <vyhodcev@internet.ru>
///
/// @copyright (c) 2024 Stilsoft
///
/// MIT License:
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the 'Software'), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.

#ifndef PARAOS_CRITICAL_HPP
#define PARAOS_CRITICAL_HPP

#include "FreeRTOS.h"
#include "paraos_attr.h"
#include "paraos_config.hpp"
#include "task.h"

namespace paraos {

/// @brief Класс-реализация критической секции в freeRTOS.
class CriticalSection final {
 public:
  /// @brief Конструктор обеспечивает автоматический вход в критическую секцию.
  /// @param is_isr
  PARAOS_INLINE_CRITICAL CriticalSection(bool is_isr = false) noexcept
      : is_isr_{is_isr} {
    if (is_isr_ == false) {
      taskENTER_CRITICAL();
    } else {
      uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    }

#ifdef paraosTRACE_ENABLE
    std::cout << "Open critical section" << std::endl;
#endif
  }

  /// @brief Деструктор обеспечивает автоматический выход из критической секции.
  ~CriticalSection() {
    if (!is_isr_) {
      taskEXIT_CRITICAL();
    } else {
      taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
    }

#ifdef paraosTRACE_ENABLE
    std::cout << "Close critical section" << std::endl;
#endif
  }

  CriticalSection(const CriticalSection &other) = delete;
  CriticalSection(CriticalSection &&other) = delete;
  auto operator=(const CriticalSection &other) = delete;
  auto operator=(const CriticalSection &&other) = delete;

 private:
  const bool is_isr_;
  UBaseType_t uxSavedInterruptStatus;
};

inline void DisableIsr() { taskENTER_CRITICAL(); }
inline void EnableIsr() { taskEXIT_CRITICAL(); }
}  // namespace paraos

#endif /* PARAOS_CRITICAL_HPP */
