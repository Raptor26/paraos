/// @file paraos_semaphore.hpp
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

#ifndef PARAOS_SEMAPHORE_HPP
#define PARAOS_SEMAPHORE_HPP

#include <pthread.h>
#include <semaphore.h>

#include "etl/atomic.h"
#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_critical.hpp"
#include "paraos_isr.hpp"
#include "paraos_utils.hpp"

namespace paraos {

struct SemaphoreAttr {
  std::size_t max_count{1u};
  std::size_t initial_value{0};
};

constexpr std::size_t initial_count = 0u;

class SemaphoreBase {
 public:
  virtual ISRbool Take(
      std::size_t timeout_ms = max_delay, bool from_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(from_isr);

    // PARAOS wrapper for POSIX not provided isr functions.
    PARAOS_CHECK_ASSERT(from_isr == false);
    bool is_sem_taken = false;
    int result = -1;
    if (timeout_ms == 0) {
      result = sem_trywait(&handle_);
    } else if (timeout_ms == max_delay) {
      result = sem_wait(&handle_);
    } else {
      timespec delay{};
      delay.tv_nsec =
          static_cast<long>(timeout_ms) * NANOSECONDS_PER_MILISECONDS;

      timespec current_time{};
      clock_gettime(CLOCK_REALTIME, &current_time);

      TimespecAdd(&current_time, &delay, &delay);

      result = sem_timedwait(&handle_, &delay);
    }

    if (result == 0) {
      is_sem_taken = true;
    }
    return is_sem_taken;
  }

  virtual ISRbool Give(bool from_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(from_isr);
    bool is_sem_given{false};
    auto result = sem_post(&handle_);

    if (result == 0) {
      is_sem_given = true;
    }
    return is_sem_given;
  }

  operator bool() const { return is_sem_created_; }

 protected:
  SemaphoreBase() = default;

  virtual ~SemaphoreBase() {
    if (is_sem_created_) {
      sem_destroy(&handle_);
    }
  }

  sem_t handle_;
  bool is_sem_created_;
};

struct SemaphoreCounting final : public SemaphoreBase {
  SemaphoreCounting(const SemaphoreAttr &attr) : SemaphoreBase{} {
    if (sem_init(&handle_, 0, attr.initial_value) == 0) {
      is_sem_created_ = true;
    }
  }

  /// @brief Semaphore deleted by ~SemaphoreBase()
  ~SemaphoreCounting() = default;
};

struct SemaphoreBinary final : public SemaphoreBase {
  SemaphoreBinary() noexcept : SemaphoreBinary{SemaphoreAttr{}} {}

  SemaphoreBinary(const SemaphoreAttr &attr) noexcept : SemaphoreBase{} {
    if (sem_init(&handle_, 0, attr.initial_value) == 0) {
      is_sem_created_ = true;
    }
  }

  ISRbool Give(bool from_isr = false) override {
    ISRbool status;

    const CriticalSection critical;
    if (!is_given_) {
      status = SemaphoreBase::Give(from_isr);

      if (status) {
        is_given_ = true;
      }
    }

    return status;
  }

  ISRbool Take(
      std::size_t timeout_ms = max_delay, bool from_isr = false) override {
    ISRbool status;

    status = SemaphoreBase::Take(timeout_ms, from_isr);

    const CriticalSection critical;
    if (status) {
      is_given_ = false;
    }

    return status;
  }

  ~SemaphoreBinary() = default;

 private:
  etl::atomic_bool is_given_{false};
};
}  // namespace paraos

#endif /* PARAOS_SEMAPHORE_HPP */
