#ifndef MUTEX_HPP
#define MUTEX_HPP

#include <cassert>

#include "paraos_critical.hpp"
#include "paraos_utils.hpp"

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

namespace paraos {

struct MutexAttr {
  bool is_binary_ = false;
};

/// @brief
/// @note Пример использования мьютексов можно найти по ссылке ниже
/// https://learn.microsoft.com/ru-ru/windows/win32/sync/using-mutex-objects
class MutexBase {
 public:
  MutexBase(const MutexAttr& attr)
      : handle_{CreateMutex(nullptr, false, nullptr)},
        is_binary_{attr.is_binary_} {
#ifdef paraosTRACE_ENABLE
    std::cout << "MutexBase Ctor" << std::endl;
#endif
  }

  MutexBase() : MutexBase(MutexAttr{}) {}

  virtual ~MutexBase() {
    assert(handle_);
    if (handle_) {
      CloseHandle(handle_);

      // need for debug only
      handle_ = nullptr;
    }

#ifdef paraosTRACE_ENABLE
    std::cout << "MutexBase Dtor" << std::endl;
#endif
  }

  operator bool() const { return handle_ != nullptr ? true : false; }

  virtual bool Lock(std::size_t timeout_ms = max_delay) {
    bool is_mutex_taken = false;

    if (WaitForSingleObject(handle_, static_cast<DWORD>(timeout_ms)) ==
        WAIT_OBJECT_0) {
      is_mutex_taken = true;
    }
    return is_mutex_taken;
  }

  virtual bool Unlock() { return static_cast<bool>(ReleaseMutex(handle_)); }

 private:
  HANDLE handle_{nullptr};
  bool is_binary_{true};
};

class BoolSafeThreadFlag {
 private:
  bool is_locked_ = false;

 public:
  /// @brief Ctor.
  /// @param
  BoolSafeThreadFlag(bool new_status) : is_locked_{new_status} {}

  /// @brief Default Ctor
  BoolSafeThreadFlag() : BoolSafeThreadFlag{false} {}

  BoolSafeThreadFlag& operator=(const BoolSafeThreadFlag& ohter) {
    const CriticalSection critical;  // RAII
    is_locked_ = ohter.is_locked_;

    return *this;
  }

  operator bool() const { return Islocked(); }

 private:
  /// @brief Safe thread setter status.
  /// @param[in] new_state: New state for safe thread update status.
  inline void SetLocked(bool new_state) {
    CriticalSection critical;  // RAII
    is_locked_ = new_state;
  }

  /// @brief Safe thread getter status.
  /// @return true or false.
  inline bool Islocked() const {
    CriticalSection critical;  // RAII
    return is_locked_;
  }
};

class MutexBaseBinary : public MutexBase {
 public:
  MutexBaseBinary() : MutexBase(MutexAttr{true}) {}

  ~MutexBaseBinary() {}

  virtual bool Lock(std::size_t timeout_ms = max_delay) override {
    bool is_current_operation_locked{false};
    if (is_locked_ == false) {
      is_current_operation_locked = MutexBase::Lock(timeout_ms);
      // We call MutexBase::Lock() if our current state "unlocked". If
      // MutexBase::Lock() returned false (from unlocked state), i don't know
      // what that mean. Try find race condition for "is_locked_" variable in
      // "MutexBaseBinary" class.
      assert(is_current_operation_locked == true);
      is_locked_ = true;
    }

    return is_current_operation_locked;
  }

  virtual bool Unlock() override {
    bool is_current_operation_unlocked{false};
    if (is_locked_ == true) {
      is_current_operation_unlocked = MutexBase::Unlock();
      // We call MutexBase::Unlock() if our current state "locked". If
      // MutexBase::Unlock() returned false (from "locked" state), i don't know
      // what that mean. Try find race condition for "is_locked_" variable in
      // "MutexBaseBinary" class.
      assert(is_current_operation_unlocked == true);
      is_locked_ = false;
    }

    return is_current_operation_unlocked;
  }

 private:
  /// @brief Safe thread flag
  BoolSafeThreadFlag is_locked_;
};
}  // namespace paraos

#endif /* MUTEX_HPP */
