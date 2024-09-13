/// @file paraos_mutex.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
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

#ifndef PARAOS_MUTEX_HPP
#define PARAOS_MUTEX_HPP

#include <cassert>

#include "paraos_bool_atomic.hpp"
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

  MutexBase(const MutexBase& other) = delete;
  MutexBase(MutexBase&& other) = delete;

  MutexBase& operator=(const MutexBase& other) = delete;
  MutexBase& operator=(MutexBase&& other) = delete;

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
  [[maybe_unused]] bool is_binary_{true};
};

class MutexBaseBinary : public MutexBase {
 public:
  MutexBaseBinary() : MutexBase(MutexAttr{true}) {}

  ~MutexBaseBinary() {}

  MutexBaseBinary(const MutexBaseBinary& other) = delete;
  MutexBaseBinary(MutexBaseBinary&& other) = delete;

  MutexBaseBinary& operator=(const MutexBaseBinary& other) = delete;
  MutexBaseBinary& operator=(MutexBaseBinary&& other) = delete;

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
  BoolAtomic is_locked_{false};
};
}  // namespace paraos

#endif /* PARAOS_MUTEX_HPP */
