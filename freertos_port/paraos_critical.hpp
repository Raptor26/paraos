#ifndef PARAOS_CRITICAL_HPP
#define PARAOS_CRITICAL_HPP

#include "FreeRTOS.h"
#include "irtos_attr.h"
#include "task.h"

namespace paraos {

/// @brief Класс-реализация критической секции в freeRTOS.
class CriticalSection final {
 public:
  /// @brief Конструктор обеспечивает автоматический вход в критическую секцию.
  /// @param is_isr
  ICORE_INLINE_CRITICAL CriticalSection(bool is_isr = false) noexcept
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
}  // namespace paraos

#endif /* PARAOS_CRITICAL_HPP */
