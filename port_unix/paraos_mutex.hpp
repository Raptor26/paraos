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

#include "etl/atomic.h"
#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_utils.hpp"

namespace paraos {

struct MutexAttr {
  bool is_binary_ = false;
};

class MutexBase {
 public:
  virtual ~MutexBase() { pthread_mutex_destroy(&m_obj_); }

  MutexBase(const MutexBase& other) = delete;
  MutexBase(MutexBase&& other) = delete;

  MutexBase& operator=(const MutexBase& other) = delete;
  MutexBase& operator=(MutexBase&& other) = delete;

  bool Lock(std::size_t timeout_ms = max_delay, bool is_isr = false) noexcept {
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
          static_cast<long>(timeout_ms) * NANOSECONDS_PER_MILISECONDS;

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

  bool Unlock(bool is_isr = false) noexcept {
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

  operator bool() const { return is_mutex_ready_; }

 protected:
  MutexBase(int kind) noexcept {
    pthread_mutexattr_t pthread_mutex_attr;
    pthread_mutexattr_init(&pthread_mutex_attr);
    pthread_mutexattr_settype(&pthread_mutex_attr, kind);
    if (pthread_mutex_init(&m_obj_, &pthread_mutex_attr) == 0) {
      is_mutex_ready_ = true;
    }
  }

 protected:
  pthread_mutex_t m_obj_;

  bool is_mutex_ready_{false};

 private:
  /// @brief If lock_cnt_ == 0, then try unlock mutex. Otherwise only return
  /// false without any action. It's need for consistent API between
  /// Unix/WinAPI/FreeRTOS
  etl::atomic_int lock_cnt_{0};
};

class Mutex final : public MutexBase {
 public:
  Mutex() : MutexBase{PTHREAD_MUTEX_NORMAL} {}

  ~Mutex() {}

  Mutex(const Mutex& other) = delete;
  Mutex(Mutex&& other) = delete;

  Mutex& operator=(const Mutex& other) = delete;
  Mutex& operator=(Mutex&& other) = delete;
};

class MutexRecursive final : public MutexBase {
 public:
  MutexRecursive() : MutexBase{PTHREAD_MUTEX_RECURSIVE} {}

  ~MutexRecursive() {}

  MutexRecursive(const MutexRecursive& other) = delete;
  MutexRecursive(Mutex&& MutexRecursive) = delete;

  MutexRecursive& operator=(const MutexRecursive& other) = delete;
  MutexRecursive& operator=(MutexRecursive&& other) = delete;
};

}  // namespace paraos

#endif /* PARAOS_MUTEX_HPP */
