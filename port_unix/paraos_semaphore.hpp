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

#include "paraos_attr.h"
#include "paraos_utils.hpp"
#include "paroas_isr.hpp"

namespace paraos {

struct SemaphoreAttr {
  std::size_t max_count{1u};
  std::size_t initial_value{0};
};

constexpr std::size_t initial_count = 0u;

class Semaphore {
 public:
  Semaphore(const SemaphoreAttr attr) noexcept {
    if (sem_init(&handle_, 0, attr.initial_value) == 0) {
      is_sem_created = true;
    }
  }

  Semaphore() noexcept : Semaphore{SemaphoreAttr{}} {}

  virtual ~Semaphore() { sem_destroy(&handle_); }

  ISRbool Take(std::size_t timeout_ms = max_delay) {
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

  ISRbool Give(bool from_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(from_isr);
    bool is_sem_given{false};
    auto result = sem_post(&handle_);

    if (result == 0) {
      is_sem_given = true;
    }
    return is_sem_given;
  }

  operator bool() const { return is_sem_created; }

 private:
  sem_t handle_;
  bool is_sem_created{false};
};

struct SemaphoreBinary final : public Semaphore {
  SemaphoreBinary() noexcept : Semaphore{} {}

  SemaphoreBinary(const SemaphoreAttr &attr) noexcept : Semaphore{} {
    PARAOS_ATTR_UNUSED_VAR(attr);
  }

  ~SemaphoreBinary() = default;
};
}  // namespace paraos

#endif /* PARAOS_SEMAPHORE_HPP */
