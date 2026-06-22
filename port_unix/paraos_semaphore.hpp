/// @file paraos_semaphore.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_SEMAPHORE_HPP
#define PARAOS_SEMAPHORE_HPP

#include <errno.h>
#include <pthread.h>
#include <unistd.h>

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
  virtual auto Take(
      paraos::delay_type timeout_ms = max_delay, bool from_isr = false)
      -> ISRbool {
    PARAOS_ATTR_UNUSED_VAR(from_isr);

    // PARAOS wrapper for POSIX not provided isr functions.
    PARAOS_CHECK_ASSERT(from_isr == false);
    bool is_sem_taken = false;
    int result = -1;
    if (timeout_ms == 0) {
#ifdef __linux__
      result = sem_trywait(&handle_);
#elif defined(__APPLE__)
      result = PthreadTakeNonBlocking();
#else
#error "Unsupported Unix-like platform"
#endif
    } else if (timeout_ms == max_delay) {
#ifdef __linux__
      result = sem_wait(&handle_);
#elif defined(__APPLE__)
      result = PthreadTakeBlocking();
#else
#error "Unsupported Unix-like platform"
#endif
    } else {
#ifdef __linux__
      auto delay = MillisecondsInTimeSpec(timeout_ms);
      timespec current_time{};
      clock_gettime(CLOCK_REALTIME, &current_time);

      TimespecAdd(&current_time, &delay, &delay);

      result = sem_timedwait(&handle_, &delay);
#elif defined(__APPLE__)
      result = PthreadTakeTimed(timeout_ms);
#else
#error "Unsupported Unix-like platform"
#endif
    }

    if (result == 0) {
      is_sem_taken = true;
    }
    return static_cast<ISRbool>(is_sem_taken);
  }

  virtual auto Give(bool from_isr = false) -> ISRbool {
    PARAOS_ATTR_UNUSED_VAR(from_isr);
    bool is_sem_given{false};
#ifdef __linux__
    auto result = sem_post(&handle_);
#elif defined(__APPLE__)
    auto result = PthreadGive();
#else
#error "Unsupported Unix-like platform"
#endif

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
#ifdef __linux__
      sem_destroy(&handle_);
#elif defined(__APPLE__)
      pthread_mutex_destroy(&sema_.mutex_);
      pthread_cond_destroy(&sema_.cond_);
#else
#error "Unsupported Unix-like platform"
#endif
      is_sem_created_ = false;
    }
  }

  /// @brief Move ctor.
  SemaphoreBase(SemaphoreBase &&other) noexcept {
    if (this != &other) {
#ifdef __linux__
      this->handle_ = other.handle_;
#elif defined(__APPLE__)
      this->sema_ = other.sema_;
#else
#error "Unsupported Unix-like platform"
#endif
      this->is_sem_created_ = other.is_sem_created_.load();
      other.is_sem_created_ = false;
    }
  }

  /// @brief Move assignment.
  auto operator=(SemaphoreBase &&other) noexcept -> SemaphoreBase & {
    if (this != &other) {
      this->~SemaphoreBase();
#ifdef __linux__
      this->handle_ = other.handle_;
#elif defined(__APPLE__)
      this->sema_ = other.sema_;
#else
#error "Unsupported Unix-like platform"
#endif
      this->is_sem_created_ = other.is_sem_created_.load();
      other.is_sem_created_ = false;
    }

    return *this;
  }

#ifdef __APPLE__
  /// @brief macOS condition-variable + counter semaphore backend.
  ///
  /// Replaces deprecated unnamed POSIX semaphores (sem_init/sem_destroy) with
  /// a pthread-based implementation. All state mutations are protected by
  /// mutex_; cond_ is signalled while holding mutex_ to avoid lost wakeups.
  struct PthreadSemaphore {
    pthread_mutex_t mutex_{};
    pthread_cond_t cond_{};
    std::size_t count_{0U};
    std::size_t max_count_{1U};
  };

  /// @brief Initialize the macOS semaphore backend.
  auto CreatePthreadSemaphore(
      std::size_t max_count, std::size_t initial_value) noexcept -> bool {
    if (pthread_mutex_init(&sema_.mutex_, nullptr) != 0) {
      return false;
    }

    if (pthread_cond_init(&sema_.cond_, nullptr) != 0) {
      pthread_mutex_destroy(&sema_.mutex_);
      return false;
    }

    sema_.max_count_ = max_count;
    sema_.count_ = initial_value;
    return true;
  }

  /// @brief Non-blocking take for timeout_ms == 0.
  auto PthreadTakeNonBlocking() noexcept -> int {
    pthread_mutex_lock(&sema_.mutex_);
    if (sema_.count_ > 0U) {
      --sema_.count_;
      pthread_mutex_unlock(&sema_.mutex_);
      return 0;
    }
    pthread_mutex_unlock(&sema_.mutex_);
    errno = EAGAIN;
    return -1;
  }

  /// @brief Blocking take for timeout_ms == max_delay.
  auto PthreadTakeBlocking() noexcept -> int {
    pthread_mutex_lock(&sema_.mutex_);
    while (sema_.count_ == 0U) {
      pthread_cond_wait(&sema_.cond_, &sema_.mutex_);
    }
    --sema_.count_;
    pthread_mutex_unlock(&sema_.mutex_);
    return 0;
  }

  /// @brief Timed take for finite timeouts.
  auto PthreadTakeTimed(std::size_t timeout_ms) noexcept -> int {
    struct timespec deadline {};
    if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
      return -1;
    }

    const auto delay = MillisecondsInTimeSpec(timeout_ms);
    TimespecAdd(&deadline, &delay, &deadline);

    pthread_mutex_lock(&sema_.mutex_);
    while (sema_.count_ == 0U) {
      const int result_code =
          pthread_cond_timedwait(&sema_.cond_, &sema_.mutex_, &deadline);
      if (result_code == ETIMEDOUT) {
        pthread_mutex_unlock(&sema_.mutex_);
        return ETIMEDOUT;
      }
      // Spurious wakeup: loop and re-check count.
    }
    --sema_.count_;
    pthread_mutex_unlock(&sema_.mutex_);
    return 0;
  }

  /// @brief Give / post one unit to the semaphore.
  auto PthreadGive() noexcept -> int {
    pthread_mutex_lock(&sema_.mutex_);
    if (sema_.count_ < sema_.max_count_) {
      ++sema_.count_;
      pthread_cond_signal(&sema_.cond_);
    }
    pthread_mutex_unlock(&sema_.mutex_);
    return 0;
  }
#endif

  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  // We can't put these variables into private section, because they're used in
  // derived classes.
#ifdef __linux__
  sem_t handle_{};
#elif defined(__APPLE__)
  PthreadSemaphore sema_{};
#else
#error "Unsupported Unix-like platform"
#endif
  etl::atomic<bool> is_sem_created_{false};
  // NOLINTEND(misc-non-private-member-variables-in-classes)
};

struct SemaphoreCounting final : public SemaphoreBase {
  explicit SemaphoreCounting(const SemaphoreAttr &attr) {
#ifdef __linux__
    if (sem_init(&handle_, 0, attr.initial_value) == 0) {
      is_sem_created_ = true;
    }
#elif defined(__APPLE__)
    if (CreatePthreadSemaphore(attr.max_count, attr.initial_value)) {
      is_sem_created_ = true;
    }
#else
#error "Unsupported Unix-like platform"
#endif
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
#ifdef __linux__
      this->handle_ = other.handle_;
#elif defined(__APPLE__)
      this->sema_ = other.sema_;
#else
#error "Unsupported Unix-like platform"
#endif
      this->is_sem_created_ = other.is_sem_created_.load();
      other.is_sem_created_ = false;
    }

    return *this;
  }

  /// @brief Semaphore non-copyable
  SemaphoreCounting(const SemaphoreCounting &other) = delete;
  auto operator=(const SemaphoreCounting &other)
      -> SemaphoreCounting & = delete;
};

struct SemaphoreBinary final : public SemaphoreBase {
  SemaphoreBinary() noexcept : SemaphoreBinary{SemaphoreAttr{}} {}

  explicit SemaphoreBinary(const SemaphoreAttr &attr) noexcept {
#ifdef __linux__
    if (sem_init(&handle_, 0, attr.initial_value) == 0) {
      is_sem_created_ = true;
    }
#elif defined(__APPLE__)
    if (CreatePthreadSemaphore(1U, attr.initial_value)) {
      is_sem_created_ = true;
    }
#else
#error "Unsupported Unix-like platform"
#endif
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

  auto Take(paraos::delay_type timeout_ms = max_delay, bool from_isr = false)
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
#ifdef __linux__
      this->handle_ = other.handle_;
#elif defined(__APPLE__)
      this->sema_ = other.sema_;
#else
#error "Unsupported Unix-like platform"
#endif
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
