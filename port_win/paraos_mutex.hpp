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

#include "etl/atomic.h"
#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_isr.hpp"
#include "paraos_utils.hpp"

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

namespace paraos {

/// @brief
/// @note Пример использования мьютексов можно найти по ссылке ниже
/// https://learn.microsoft.com/ru-ru/windows/win32/sync/using-mutex-objects
class MutexBase {
 public:
  virtual ~MutexBase() {
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

  ISRbool Lock(std::size_t timeout_ms = max_delay, bool is_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    PARAOS_CHECK_ASSERT(handle_);
    ISRbool is_mutex_taken;
    bool is_need_take{false};

    if (((!is_recursive_) && (lock_cnt_ == 0)) || is_recursive_) {
      is_need_take = true;
    }

    if (is_need_take) {
      if (WaitForSingleObject(handle_, static_cast<DWORD>(timeout_ms)) ==
          WAIT_OBJECT_0) {
        is_mutex_taken.is_success_ = true;
        ++lock_cnt_;
      }
    }

    return is_mutex_taken;
  }

  ISRbool Unlock(bool is_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    PARAOS_CHECK_ASSERT(handle_);

    ISRbool is_unlock = static_cast<ISRbool>(ReleaseMutex(handle_));
    if (is_unlock == true) {
      --lock_cnt_;
    }

    return is_unlock;
  }

 protected:
  MutexBase(bool is_recursive) : is_recursive_{is_recursive} {
    handle_ = CreateMutex(nullptr, false, nullptr);
  };

  /// @brief Move Ctor,
  MutexBase(MutexBase&& other) {
    if (this != &other) {
      this->handle_ = other.handle_;
      this->is_recursive_ = other.is_recursive_.load();
      this->lock_cnt_ = other.lock_cnt_.load();

      other.handle_ = nullptr;
    }
  }

  /// @brief Move assignment.
  MutexBase& operator=(MutexBase&& other) {
    if (this == &other) {
      return *this;
    }

    this->~MutexBase();
    this->handle_ = other.handle_;
    this->is_recursive_ = other.is_recursive_.load();
    this->lock_cnt_ = other.lock_cnt_.load();

    other.handle_ = nullptr;

    return *this;
  }

  /// @brief  Mutex non-copyable
  MutexBase& operator=(const MutexBase& other) = delete;
  MutexBase(const MutexBase& other) = delete;

 protected:
  HANDLE handle_{nullptr};

  etl::atomic<bool> is_recursive_{false};
  etl::atomic<int> lock_cnt_{0};
};

class Mutex final : public MutexBase {
 public:
  Mutex() : MutexBase{false} {}

  ~Mutex() = default;

  /// @brief Move Ctor,
  Mutex(Mutex&& other) : MutexBase(std::move(other)) {};

  /// @brief Move assignment.
  Mutex& operator=(Mutex&& other) {
    if (this == &other) {
      return *this;
    }

    this->~Mutex();
    this->handle_ = other.handle_;
    this->is_recursive_ = other.is_recursive_.load();
    this->lock_cnt_ = other.lock_cnt_.load();

    other.handle_ = nullptr;

    return *this;
  }

  /// @brief  Mutex non-copyable.
  Mutex& operator=(const Mutex& other) = delete;
  Mutex(const Mutex& other) = delete;
};

/// @brief
/// @see Why recursive mutex is evil:
/// https://stackoverflow.com/questions/2323490/non-recursive-mutex-ownership
class MutexRecursive final : public MutexBase {
 public:
  MutexRecursive() : MutexBase{true} {}

  ~MutexRecursive() = default;

  /// @brief Move Ctor,
  MutexRecursive(MutexRecursive&& other) : MutexBase(std::move(other)) {};

  /// @brief Move assignment.
  MutexRecursive& operator=(MutexRecursive&& other) {
    if (this == &other) {
      return *this;
    }

    this->~MutexRecursive();
    this->handle_ = other.handle_;
    this->is_recursive_ = other.is_recursive_.load();
    this->lock_cnt_ = other.lock_cnt_.load();

    other.handle_ = nullptr;

    return *this;
  }

  /// @brief  Mutex non-copyable.
  MutexRecursive(const MutexRecursive& other) = delete;
  MutexRecursive& operator=(const MutexRecursive& other) = delete;
};

}  // namespace paraos

#endif /* PARAOS_MUTEX_HPP */
