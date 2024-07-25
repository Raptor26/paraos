#ifndef CRITICAL_HPP
#define CRITICAL_HPP

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

#include "paraos_mutex.hpp"

namespace paraos {

class CriticalSectionFactory final {
 public:
  CriticalSectionFactory() noexcept {}

  ~CriticalSectionFactory() {}

 private:
};

class CriticalSection final {
 public:
  /// @brief Конструктор обеспечивает автоматический вход в критическую секцию.
  /// @param is_isr
  CriticalSection(bool is_isr = false) : is_isr_{is_isr} { mutex_.Lock(); }

  /// @brief Деструктор обеспечивает автоматический выход из критической секции.
  ~CriticalSection() { mutex_.Unlock(); }

 private:
  const bool is_isr_;
  static inline MutexBase mutex_;
};

}  // namespace paraos

#endif /* CRITICAL_HPP */
