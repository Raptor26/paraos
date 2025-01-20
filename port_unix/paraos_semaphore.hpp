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

#include <utility>

#include "etl/atomic.h"
#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_critical.hpp"
#include "paraos_isr.hpp"
#include "paraos_utils.hpp"

namespace paraos {

struct SemaphoreAttr {
  std::size_t max_count{1U};
  std::size_t initial_value{0U};
};

constexpr std::size_t initial_count = 0U;

class SemaphoreBase {
 public:
  // Unix specific semaphore realization.
  // NOLINTBEGIN(google-default-arguments)
  virtual auto Take(std::size_t timeout_ms = max_delay, bool from_isr = false)
      -> ISRbool {
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
          static_cast<int64_t>(timeout_ms) * NANOSECONDS_PER_MILISECONDS;

      timespec current_time{};
      clock_gettime(CLOCK_REALTIME, &current_time);

      TimespecAdd(&current_time, &delay, &delay);

      result = sem_timedwait(&handle_, &delay);
    }

    if (result == 0) {
      is_sem_taken = true;
    }
    return static_cast<ISRbool>(is_sem_taken);
  }

  virtual auto Give(bool from_isr = false) -> ISRbool {
    PARAOS_ATTR_UNUSED_VAR(from_isr);
    bool is_sem_given{false};
    auto result = sem_post(&handle_);

    if (result == 0) {
      is_sem_given = true;
    }
    return static_cast<ISRbool>(is_sem_given);
  }
  // NOLINTEND(google-default-arguments)

  explicit operator bool() const { return is_sem_created_; }

  /// @brief Semaphore non-copyable
  SemaphoreBase(const SemaphoreBase &other) = delete;
  auto operator=(const SemaphoreBase &other) -> SemaphoreBase & = delete;

 protected:
  SemaphoreBase() = default;

  virtual ~SemaphoreBase() {
    if (is_sem_created_) {
      sem_destroy(&handle_);
      is_sem_created_ = false;
    }
  }

  /// @brief Move ctor.
  SemaphoreBase(SemaphoreBase &&other) noexcept {
    if (this != &other) {
      this->handle_ = other.handle_;
      this->is_sem_created_ = other.is_sem_created_.load();
      other.is_sem_created_ = false;
    }
  }

  /// @brief Move assignment.
  auto operator=(SemaphoreBase &&other) noexcept -> SemaphoreBase & {
    if (this != &other) {
      this->~SemaphoreBase();
      this->handle_ = other.handle_;
      this->is_sem_created_ = other.is_sem_created_.load();
      other.is_sem_created_ = false;
    }

    return *this;
  }

  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  // We can't put these variables into private section, because they're used in
  // derived classes.
  sem_t handle_{};
  etl::atomic<bool> is_sem_created_{false};
  // NOLINTEND(misc-non-private-member-variables-in-classes)
};

struct SemaphoreCounting final : public SemaphoreBase {
  explicit SemaphoreCounting(const SemaphoreAttr &attr) {
    if (sem_init(&handle_, 0, attr.initial_value) == 0) {
      is_sem_created_ = true;
    }
  }

  /// @brief Semaphore deleted by ~SemaphoreBase()
  ~SemaphoreCounting() override = default;

  /// @brief Move ctor.
  SemaphoreCounting(SemaphoreCounting &&other) noexcept
      : SemaphoreBase(std::move(other)) {}

  /// @brief Move assignment.
  auto operator=(SemaphoreCounting &&other) noexcept -> SemaphoreCounting & {
    if (this != &other) {
      this->~SemaphoreCounting();
      this->handle_ = other.handle_;
      this->is_sem_created_ = other.is_sem_created_.load();
      other.is_sem_created_ = false;
    }

    return *this;
  }

  /// @brief Semaphore non-copyable
  SemaphoreCounting(const SemaphoreCounting &other) = delete;
  auto operator=(const SemaphoreCounting &other) -> SemaphoreCounting & =
                                                        delete;
};

struct SemaphoreBinary final : public SemaphoreBase {
  SemaphoreBinary() noexcept : SemaphoreBinary{SemaphoreAttr{}} {}

  explicit SemaphoreBinary(const SemaphoreAttr &attr) noexcept {
    if (sem_init(&handle_, 0, attr.initial_value) == 0) {
      is_sem_created_ = true;
    }
  }

  // Unix specific semaphore realization.
  // NOLINTBEGIN(google-default-arguments)
  auto Give(bool from_isr = false) -> ISRbool override {
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

  auto Take(std::size_t timeout_ms = max_delay, bool from_isr = false)
      -> ISRbool override {
    ISRbool status;

    status = SemaphoreBase::Take(timeout_ms, from_isr);

    const CriticalSection critical;
    if (status) {
      is_given_ = false;
    }

    return status;
  }
  // NOLINTEND(google-default-arguments)

  ~SemaphoreBinary() override = default;

  /// @brief Move ctor.
  SemaphoreBinary(SemaphoreBinary &&other) noexcept
      : SemaphoreBase(std::move(other)) {}

  /// @brief Move assignment.
  auto operator=(SemaphoreBinary &&other) noexcept -> SemaphoreBinary & {
    if (this != &other) {
      this->~SemaphoreBinary();
      this->handle_ = other.handle_;
      this->is_sem_created_ = other.is_sem_created_.load();
      this->is_given_ = other.is_given_.load();

      other.is_sem_created_ = false;
      other.is_given_ = false;
    }

    return *this;
  }

  /// @brief Semaphore non-copyable
  SemaphoreBinary(const SemaphoreBinary &other) = delete;
  auto operator=(const SemaphoreBinary &other) -> SemaphoreBinary & = delete;

 private:
  etl::atomic<bool> is_given_{false};
};
}  // namespace paraos

#endif /* PARAOS_SEMAPHORE_HPP */
