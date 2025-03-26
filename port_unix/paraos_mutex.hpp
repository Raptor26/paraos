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

#include <pthread.h>

#include <atomic>

#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_utils.hpp"

namespace paraos {

struct MutexAttr {
  bool is_binary_ = false;
};

class MutexBase {
 public:
  virtual ~MutexBase() {
    if (is_mutex_ready_) {
      pthread_mutex_destroy(&m_obj_);
      is_mutex_ready_ = false;
    }
  }

  auto Lock(std::size_t timeout_ms = max_delay, bool is_isr = false) noexcept
      -> bool {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    bool is_mutex_taken{false};

    int result{-1};
    if (timeout_ms == 0) {
      result = pthread_mutex_trylock(&m_obj_);
    } else if (timeout_ms == max_delay) {
      result = pthread_mutex_lock(&m_obj_);
    } else {
      timespec delay{};
      delay.tv_nsec =
          static_cast<int64_t>(timeout_ms) * NANOSECONDS_PER_MILISECONDS;

      timespec current_time{};
      clock_gettime(CLOCK_REALTIME, &current_time);

      TimespecAdd(&current_time, &delay, &delay);

      result = pthread_mutex_timedlock(&m_obj_, &delay);
    }

    if (result == 0) {
      ++lock_cnt_;
      is_mutex_taken = true;
    }

    return is_mutex_taken;
  }

  auto Unlock(bool is_isr = false) noexcept -> bool {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    bool is_mutex_released{false};

    // Paraos mutex API need return false if unlock mutex operations cnt greater
    // then lock.
    if (lock_cnt_ > 0) {
      if (pthread_mutex_unlock(&m_obj_) == 0) {
        --lock_cnt_;
        is_mutex_released = true;
      }
    }

    return is_mutex_released;
  }

  explicit operator bool() const { return is_mutex_ready_; }

  /// @brief  Mutex non-copyable
  auto operator=(const MutexBase& other) -> MutexBase& = delete;
  MutexBase(const MutexBase& other) = delete;

 protected:
  explicit MutexBase(int kind) noexcept {
    pthread_mutexattr_t pthread_mutex_attr;
    pthread_mutexattr_init(&pthread_mutex_attr);
    pthread_mutexattr_settype(&pthread_mutex_attr, kind);
    if (pthread_mutex_init(&m_obj_, &pthread_mutex_attr) == 0) {
      is_mutex_ready_ = true;
    }
  }

  /// @brief Move Ctor,
  MutexBase(MutexBase&& other) noexcept {
    if (this != &other) {
      this->m_obj_ = other.m_obj_;
      this->lock_cnt_ = other.lock_cnt_.load();
      this->is_mutex_ready_ = other.is_mutex_ready_.load();

      other.is_mutex_ready_ = false;
    }
  }

  /// @brief Move assignment.
  auto operator=(MutexBase&& other) noexcept -> MutexBase& {
    if (this == &other) {
      return *this;
    }

    this->~MutexBase();
    this->m_obj_ = other.m_obj_;
    this->lock_cnt_ = other.lock_cnt_.load();
    this->is_mutex_ready_ = other.is_mutex_ready_.load();

    other.is_mutex_ready_ = false;

    return *this;
  }

 protected:
  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  // We can't put these variables into private section, because they're used in
  // derived classes.
  pthread_mutex_t m_obj_{};

  std::atomic<bool> is_mutex_ready_{false};

  /// @brief If lock_cnt_ == 0, then try unlock mutex. Otherwise only return
  /// false without any action. It's need for consistent API between
  /// Unix/WinAPI/FreeRTOS
  std::atomic<int> lock_cnt_{0};
  // NOLINTEND(misc-non-private-member-variables-in-classes)
};

class Mutex final : public MutexBase {
 public:
  Mutex() : MutexBase{PTHREAD_MUTEX_NORMAL} {}

  ~Mutex() override = default;

  /// @brief Move Ctor,
  Mutex(Mutex&& other) noexcept : MutexBase(std::move(other)){};

  /// @brief Move assignment.
  auto operator=(Mutex&& other) noexcept -> Mutex& {
    if (this == &other) {
      return *this;
    }

    this->~Mutex();
    this->m_obj_ = other.m_obj_;
    this->lock_cnt_ = other.lock_cnt_.load();
    this->is_mutex_ready_ = other.is_mutex_ready_.load();

    other.is_mutex_ready_ = false;

    return *this;
  }

  /// @brief  Mutex non-copyable.
  auto operator=(const Mutex& other) -> Mutex& = delete;
  Mutex(const Mutex& other) = delete;
};

class MutexRecursive final : public MutexBase {
 public:
  MutexRecursive() : MutexBase{PTHREAD_MUTEX_RECURSIVE} {}

  ~MutexRecursive() override = default;

  /// @brief Move Ctor,
  MutexRecursive(MutexRecursive&& other) noexcept
      : MutexBase(std::move(other)){};

  /// @brief Move assignment.
  auto operator=(MutexRecursive&& other) noexcept -> MutexRecursive& {
    if (this == &other) {
      return *this;
    }

    this->~MutexRecursive();
    this->m_obj_ = other.m_obj_;
    this->lock_cnt_ = other.lock_cnt_.load();
    this->is_mutex_ready_ = other.is_mutex_ready_.load();

    other.is_mutex_ready_ = false;

    return *this;
  }

  /// @brief  Mutex non-copyable.
  MutexRecursive(const MutexRecursive& other) = delete;
  auto operator=(const MutexRecursive& other) -> MutexRecursive& = delete;
};

}  // namespace paraos

#endif /* PARAOS_MUTEX_HPP */
