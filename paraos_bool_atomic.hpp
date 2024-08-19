#ifndef PARAOS_BOOL_ATOMIC_HPP
#define PARAOS_BOOL_ATOMIC_HPP

#include "paraos_critical.hpp"

namespace paraos {

class BoolSafeThreadFlag {
 public:
  /// @brief Ctor.
  /// @param
  BoolSafeThreadFlag(bool new_status) noexcept : is_locked_{new_status} {}

  /// @brief Default Ctor
  BoolSafeThreadFlag() noexcept : BoolSafeThreadFlag{false} {}

  // ---------------------------------------------------------------------------
  // Five Rule
  // ---------------------------------------------------------------------------

  BoolSafeThreadFlag(const BoolSafeThreadFlag& other) noexcept {
    const paraos::CriticalSection critical;  // RAII
    is_locked_ = other.is_locked_;
  }

  BoolSafeThreadFlag(BoolSafeThreadFlag&& other) noexcept {
    const paraos::CriticalSection critical;  // RAII
    is_locked_ = other.is_locked_;
  }

  BoolSafeThreadFlag& operator=(const BoolSafeThreadFlag& other) noexcept {
    if (this != &other) {
      const paraos::CriticalSection critical;  // RAII
      is_locked_ = other.is_locked_;
    }

    return *this;
  }

  BoolSafeThreadFlag& operator=(BoolSafeThreadFlag&& other) {
    const paraos::CriticalSection critical;  // RAII
    is_locked_ = other.is_locked_;

    return *this;
  }

  operator bool() const noexcept { return Islocked(); }

 private:
  /// @brief Safe thread setter status.
  /// @param[in] new_state: New state for safe thread update status.
  inline void SetLocked(bool new_state) noexcept {
    const paraos::CriticalSection critical;  // RAII
    is_locked_ = new_state;
  }

  /// @brief Safe thread getter status.
  /// @return true or false.
  inline bool Islocked() const noexcept {
    const paraos::CriticalSection critical;  // RAII
    return is_locked_;
  }

 private:
  bool is_locked_ = false;
};

}  // namespace paraos

#endif /* PARAOS_BOOL_ATOMIC_HPP */
