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
    if (handle_ != nullptr) {
      CloseHandle(handle_);

      // need for debug only
      handle_ = nullptr;
    }
  }

  explicit operator bool() const {
    return static_cast<bool>(handle_ != nullptr);
  }

  auto Lock(std::size_t timeout_ms = max_delay, bool is_isr = false)
      -> ISRbool {
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
        is_mutex_taken.SetSuccessStatus(true);
        ++lock_cnt_;
      }
    }

    return is_mutex_taken;
  }

  auto Unlock(bool is_isr = false) -> ISRbool {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    PARAOS_CHECK_ASSERT(handle_);

    auto is_unlock =
        static_cast<ISRbool>(static_cast<bool>(ReleaseMutex(handle_)));
    if (static_cast<bool>(is_unlock)) {
      --lock_cnt_;
    }

    return is_unlock;
  }

  /// @brief  Mutex non-copyable
  auto operator=(const MutexBase& other) -> MutexBase& = delete;
  MutexBase(const MutexBase& other) = delete;

 protected:
  explicit MutexBase(bool is_recursive) : is_recursive_{is_recursive} {
    handle_ = CreateMutex(nullptr, 0, nullptr);
  };

  /// @brief Move Ctor,
  MutexBase(MutexBase&& other) noexcept {
    if (this != &other) {
      this->handle_ = other.handle_;
      this->is_recursive_ = other.is_recursive_.load();
      this->lock_cnt_ = other.lock_cnt_.load();

      other.handle_ = nullptr;
    }
  }

  /// @brief Move assignment.
  auto operator=(MutexBase&& other) noexcept -> MutexBase& {
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

 protected:
  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  // We can't put these variables into private section, because they're used in
  // derived classes.
  HANDLE handle_{nullptr};

  etl::atomic<bool> is_recursive_{false};
  etl::atomic<int> lock_cnt_{0};
  // NOLINTEND(misc-non-private-member-variables-in-classes)
};

class Mutex final : public MutexBase {
 public:
  Mutex() : MutexBase{false} {}

  ~Mutex() override = default;

  /// @brief Move Ctor,
  Mutex(Mutex&& other) noexcept : MutexBase(std::move(other)) {};

  /// @brief Move assignment.
  auto operator=(Mutex&& other) noexcept -> Mutex& {
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
  auto operator=(const Mutex& other) -> Mutex& = delete;
  Mutex(const Mutex& other) = delete;
};

/// @brief
/// @see Why recursive mutex is evil:
/// https://stackoverflow.com/questions/2323490/non-recursive-mutex-ownership
class MutexRecursive final : public MutexBase {
 public:
  MutexRecursive() : MutexBase{true} {}

  ~MutexRecursive() override = default;

  /// @brief Move Ctor,
  MutexRecursive(MutexRecursive&& other) noexcept
      : MutexBase(std::move(other)) {};

  /// @brief Move assignment.
  auto operator=(MutexRecursive&& other) noexcept -> MutexRecursive& {
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
  auto operator=(const MutexRecursive& other) -> MutexRecursive& = delete;
};

}  // namespace paraos

#endif /* PARAOS_MUTEX_HPP */
