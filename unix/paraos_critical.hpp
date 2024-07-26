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

class BoolSafeThreadFlag {
 private:
  bool is_locked_ = false;

 public:
  /// @brief Ctor.
  /// @param
  BoolSafeThreadFlag(bool new_status) noexcept : is_locked_{new_status} {}

  /// @brief Default Ctor
  BoolSafeThreadFlag() noexcept : BoolSafeThreadFlag{false} {}

  BoolSafeThreadFlag& operator=(const BoolSafeThreadFlag& other) noexcept {
    if (this != &other) {
      const CriticalSection critical;  // RAII
      is_locked_ = other.is_locked_;
    }

    return *this;
  }

  operator bool() const noexcept { return Islocked(); }

 private:
  /// @brief Safe thread setter status.
  /// @param[in] new_state: New state for safe thread update status.
  inline void SetLocked(bool new_state) noexcept {
    const CriticalSection critical;  // RAII
    is_locked_ = new_state;
  }

  /// @brief Safe thread getter status.
  /// @return true or false.
  inline bool Islocked() const noexcept {
    const CriticalSection critical;  // RAII
    return is_locked_;
  }
};

}  // namespace paraos

#endif /* CRITICAL_HPP */
