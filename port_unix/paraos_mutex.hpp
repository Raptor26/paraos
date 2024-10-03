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

#include "paraos_check.h"
#include "paraos_trace.hpp"
#include "paraos_utils.hpp"

namespace paraos {

struct MutexAttr {
  bool is_binary_ = false;
};

class MutexBase {
 public:
  MutexBase(const MutexAttr& attr) noexcept : is_binary_{attr.is_binary_} {
    if (!is_init_) {
      pthread_mutexattr_t pthread_mutex_attr;
      pthread_mutexattr_init(&pthread_mutex_attr);

      if (attr.is_binary_ == false) {
        pthread_mutexattr_settype(&pthread_mutex_attr, PTHREAD_MUTEX_RECURSIVE);
      }

      auto status = pthread_mutex_init(&m_obj_, &pthread_mutex_attr);

      if (status == 0) {
        paraosTRACE_MESSAGE("Mutex constructed");
        is_init_ = true;
      }
    } else {
      PARAOS_CHECK_ASSERT(
          false && "Don't call second init for initalized object");
    }
  }

  MutexBase() noexcept : MutexBase(MutexAttr{}) {}

  virtual ~MutexBase() {
    pthread_mutex_destroy(&m_obj_);

    paraosTRACE_MESSAGE("Mutex deleted");
    // Debug only
    is_init_ = false;
  }

  /// @brief Возвращает true если мьютекс успешно инициализирован.
  /// @note Рекомендуется выполнить данную проверку единожды после создания
  /// мьютекса.
  operator bool() const noexcept { return is_init_; }

  MutexBase(const MutexBase& other) = delete;
  MutexBase(MutexBase&& other) = delete;

  MutexBase& operator=(const MutexBase& other) = delete;
  MutexBase& operator=(MutexBase&& other) = delete;

  virtual bool Lock(std::size_t timeout_ms = max_delay) noexcept {
    bool is_mutex_taken{false};

    int result = -1;
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
      is_mutex_taken = true;
      paraosTRACE_MESSAGE("Mutex locked");
    }

    return is_mutex_taken;
  }

  virtual bool Unlock() noexcept {
    bool is_mutex_released{false};

    auto result = pthread_mutex_unlock(&m_obj_);
    if (result == 0) {
      is_mutex_released = true;
      paraosTRACE_MESSAGE("Mutex unlocked");
    }

    return is_mutex_released;
  }

 private:
  pthread_mutex_t m_obj_;
  [[maybe_unused]] bool is_binary_{true};
  bool is_init_{false};
};

class MutexBaseBinary : public MutexBase {
 public:
  MutexBaseBinary() : MutexBase(MutexAttr{true}) {}

  ~MutexBaseBinary() {}

  MutexBaseBinary(const MutexBaseBinary& other) = delete;
  MutexBaseBinary(MutexBaseBinary&& other) = delete;

  MutexBaseBinary& operator=(const MutexBaseBinary& other) = delete;
  MutexBaseBinary& operator=(MutexBaseBinary&& other) = delete;
};

}  // namespace paraos

#endif /* PARAOS_MUTEX_HPP */
