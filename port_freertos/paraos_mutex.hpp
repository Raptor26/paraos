/// @file paraos_mutex.hpp
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

#ifndef PARAOS_MUTEX_HPP
#define PARAOS_MUTEX_HPP

#include <stddef.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "etl/atomic.h"
#include "paraos_bool_atomic.hpp"
#include "paraos_check.h"
#include "paraos_critical.hpp"
#include "paraos_isr.hpp"
#include "paraos_utils.hpp"
#include "semphr.h"

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
      vSemaphoreDelete(handle_);

      // need for debug only
      handle_ = nullptr;
    }
  }

  operator bool() const { return handle_ != nullptr ? true : false; }

  ISRbool Lock(std::size_t timeout_ms = max_delay, bool is_isr = false) {
    PARAOS_CHECK_ASSERT(handle_);
    ISRbool is_mutex_taken;

    if (!is_isr) {
      if (is_recursive_) {
        // Lock recursive can take mutex if Lock called by mutex holder thread
        // or another.
        is_mutex_taken = LockRecursive(timeout_ms);
      } else {
        is_mutex_taken = LockNormal(timeout_ms);
      }
    } else {
      is_mutex_taken = LockIsr();
    }

    return is_mutex_taken;
  }

  ISRbool Unlock(bool is_isr = false) {
    PARAOS_CHECK_ASSERT(handle_);
    ISRbool is_mutex_release;

    if (!is_isr) {
      // UnlockRecursive() may call only if caller thread is mutex holder.
      // Otherwise try release as normal mutex.
      if ((is_recursive_ == true) &&
          (xTaskGetCurrentTaskHandle() == xSemaphoreGetMutexHolder(handle_))) {
        // Recursive mutex release operations counter can't be greater then take
        // operations counter.
        if (recursive_holder_take_cnt_ > 0) {
          is_mutex_release = UnlockRecursive();
        }
      } else {
        // Mutex releases a thread that is not mutex holder.
        is_mutex_release = UnlockNormal();
      }
    } else {
      is_mutex_release = UnlockIsr();
    }

    return is_mutex_release;
  }

 protected:
  MutexBase(bool is_recursive = false)
      : is_recursive_{is_recursive}, recursive_holder_take_cnt_{0} {};

  /// @brief Move Ctor,
  MutexBase(MutexBase&& other) {
    if (this != &other) {
      paraos::CriticalSection critical;
      this->handle_ = other.handle_;
      this->is_recursive_ = other.is_recursive_.load();
      this->recursive_holder_take_cnt_ =
          other.recursive_holder_take_cnt_.load();

      other.handle_ = nullptr;
    }
  }

  /// @brief Move assignment.
  MutexBase& operator=(MutexBase&& other) {
    if (this == &other) {
      return *this;
    }

    paraos::CriticalSection critical;
    this->~MutexBase();
    this->handle_ = other.handle_;
    this->is_recursive_ = other.is_recursive_.load();
    this->recursive_holder_take_cnt_ = other.recursive_holder_take_cnt_.load();

    other.handle_ = nullptr;

    return *this;
  }

  /// @brief  Mutex non-copyable
  MutexBase& operator=(const MutexBase& other) = delete;
  MutexBase(const MutexBase& other) = delete;

 private:
  ISRbool LockNormal(std::size_t timeout_ms = max_delay) {
    ISRbool is_mutex_taken;
    if (xSemaphoreTake(handle_, PARAOS_ConvertMsToTicks(timeout_ms)) ==
        pdTRUE) {
      is_mutex_taken.is_success_ = true;
    }

    return is_mutex_taken;
  }

  ISRbool LockRecursive(std::size_t timeout_ms = max_delay) {
    ISRbool is_mutex_taken;
    if (xSemaphoreGetMutexHolder(handle_) == xTaskGetCurrentTaskHandle()) {
      ++recursive_holder_take_cnt_;
    }
    if (xQueueTakeMutexRecursive(
            handle_, PARAOS_ConvertMsToTicks(timeout_ms)) == pdTRUE) {
      is_mutex_taken.is_success_ = true;
    }

    return is_mutex_taken;
  }

  ISRbool LockIsr() {
    ISRbool is_mutex_taken;
    BaseType_t xHigherPriorityTaskWoken{pdFALSE};
    if (xSemaphoreTakeFromISR(handle_, &xHigherPriorityTaskWoken) == pdTRUE) {
      is_mutex_taken.is_success_ = true;
      if (xHigherPriorityTaskWoken == pdTRUE) {
        is_mutex_taken.is_need_switch_context_ = true;
      }
    }

    return is_mutex_taken;
  }

  ISRbool UnlockRecursive() {
    ISRbool is_mutex_release;
    if (xSemaphoreGiveRecursive(handle_) == pdTRUE) {
      is_mutex_release.is_success_ = true;
      --recursive_holder_take_cnt_;
    }

    return is_mutex_release;
  }

  ISRbool UnlockNormal() {
    ISRbool is_mutex_release;
    if (xSemaphoreGive(handle_) == pdTRUE) {
      is_mutex_release.is_success_ = true;
    }
    return is_mutex_release;
  }

  ISRbool UnlockIsr() {
    ISRbool is_mutex_release;
    BaseType_t xHigherPriorityTaskWoken{pdFALSE};
    if (xSemaphoreGiveFromISR(handle_, &xHigherPriorityTaskWoken) == pdTRUE) {
      is_mutex_release.is_success_ = true;
      if (xHigherPriorityTaskWoken == pdTRUE) {
        is_mutex_release.is_need_switch_context_ = true;
      }
    }

    return is_mutex_release;
  }

 protected:
  SemaphoreHandle_t handle_{nullptr};
  etl::atomic<bool> is_recursive_;

  /// @brief Watch for symmetric call Lock() and Unlock() for recursive mutex.
  etl::atomic<size_t> recursive_holder_take_cnt_;
};

class Mutex final : public MutexBase {
 public:
  Mutex() : MutexBase{false} { handle_ = xSemaphoreCreateMutex(); }

  ~Mutex() = default;

  /// @brief Move Ctor,
  Mutex(Mutex&& other) : MutexBase(std::move(other)) {};

  /// @brief Move assignment.
  Mutex& operator=(Mutex&& other) {
    if (this == &other) {
      return *this;
    }

    paraos::CriticalSection critical;
    this->~Mutex();
    this->handle_ = other.handle_;
    this->is_recursive_ = other.is_recursive_.load();
    this->recursive_holder_take_cnt_ = other.recursive_holder_take_cnt_.load();

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
  MutexRecursive() : MutexBase{true} {
    handle_ = xSemaphoreCreateRecursiveMutex();
  }

  ~MutexRecursive() = default;

  /// @brief Move Ctor,
  MutexRecursive(MutexRecursive&& other) : MutexBase(std::move(other)) {};

  /// @brief Move assignment.
  MutexRecursive& operator=(MutexRecursive&& other) {
    if (this == &other) {
      return *this;
    }

    paraos::CriticalSection critical;
    this->~MutexRecursive();
    this->handle_ = other.handle_;
    this->is_recursive_ = other.is_recursive_.load();
    this->recursive_holder_take_cnt_ = other.recursive_holder_take_cnt_.load();

    other.handle_ = nullptr;

    return *this;
  }

  /// @brief  Mutex non-copyable.
  MutexRecursive(const MutexRecursive& other) = delete;
  MutexRecursive& operator=(const MutexRecursive& other) = delete;
};

}  // namespace paraos

#endif /* PARAOS_MUTEX_HPP */
