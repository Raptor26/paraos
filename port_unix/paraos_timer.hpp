/// @file paraos_timer.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_TIMER_HPP
#define PARAOS_TIMER_HPP

#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <cstring>
#include <limits>
#include <string_view>

#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_isr.hpp"
#include "paraos_utils.hpp"

namespace paraos {

/// @brief Class provided software timers. For creating timer, user code must
/// provide custom class as derived from paraos::Timer.
///
/// @see
/// https://stackoverflow.com/questions/64429205/how-to-use-sigev-thread-sigevent-for-linux-timers-expiration-handling-in-c
///
/// @example See example of usages software timers in
/// <example_paraos_timer.cpp>.
///

class Timer {
 public:
  /// @brief Software timer ctor.
  /// @param[in] period_ms: Period in microseconds between timer scheduler
  /// calling overriden by user method Run().
  /// @param[in] start_immediately: If set true, user code don't need call
  /// Start() for start timer. In otherwise, user must call Start() for run
  /// timer.
  /// @param[in] is_auto_reload: If set true, overriden by user method Run()
  /// will call periodical with period_ms respect. If set false, Run() will call
  /// at ones after period_ms delay. If is_auto_reload == false and user code
  /// needs call Run() again, call Start().
  /// @param[in] name: Human readable string. Useful for debug.
  explicit Timer(
      std::size_t period_ms, bool start_immediately = false,
      bool is_auto_reload = true, std::string_view name = "Timer")
      : period_ms_{period_ms}, is_auto_reload_{is_auto_reload}, name_{name} {
    Create();
    if (start_immediately) {
      Start();
    }
  }

  virtual ~Timer() {
#ifdef __linux__
    auto status = timer_delete(timer_id_);
    PARAOS_CHECK_ASSERT(status == 0);
    PARAOS_ATTR_UNUSED_VAR(status);
#elif defined(__APPLE__)
    Stop();
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&cond_);
#else
#error "Unsupported Unix-like platform"
#endif
  }

  /// @brief Start timer. If is_auto_reload was set in Ctor, then Run() method
  /// will call only once after <period_ms> delay. In other case, Run() will
  /// called periodically with <period_ms> delay respect. For change period
  /// use ChangePeriod().
  /// @param[in] max_block_time: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @param[in] is_isr: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @return Return true is timer successfully started, false in otherwise.
  auto Start(paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> ISRbool {
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    ISRbool is_timer_started{false};

#ifdef __linux__
    struct itimerspec itval{};

    if (is_auto_reload_) {
      itval.it_value = MillisecondsInTimeSpec(period_ms_);
      itval.it_interval.tv_sec = itval.it_value.tv_sec;
      itval.it_interval.tv_nsec = itval.it_value.tv_nsec;
    } else {
      itval.it_value.tv_nsec = 1U;
    }

    auto status = timer_settime(timer_id_, 0, &itval, nullptr);
    if (status == 0) {
      is_timer_started.SetSuccessStatus(true);
    }
#elif defined(__APPLE__)
    pthread_mutex_lock(&mutex_);
    if (is_running_) {
      // Timer is already running. Notify the worker so it recomputes the
      // deadline using the current period_ms_. This covers ChangePeriod()
      // and Reset() semantics.
      pthread_cond_signal(&cond_);
      is_timer_started.SetSuccessStatus(true);
    } else {
      is_stop_requested_ = false;
      is_running_ = true;
      if (pthread_create(&thread_, nullptr, ThreadRoutine,
                         static_cast<void *>(this)) == 0) {
        is_timer_started.SetSuccessStatus(true);
      } else {
        is_running_ = false;
      }
    }
    pthread_mutex_unlock(&mutex_);
#else
#error "Unsupported Unix-like platform"
#endif

    return is_timer_started;
  }

  /// @brief Change period between periodically call Run() if timer mode
  /// periodical (is_auto_reload == true), or changed delay before scheduler
  /// call Run() after user call Start() if one shot timer mode (is_auto_reload
  /// == false).
  /// @param[in] period_ms: New value for period update.
  /// @param[in] max_block_time: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @param[in] is_isr: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @return Return true if period update successfully, false in otherwise.
  auto ChangePeriod(
      std::size_t period_ms, paraos::delay_type max_block_time = max_delay,
      bool is_isr = false) -> ISRbool {
    period_ms_ = period_ms;

    return Start(max_block_time, is_isr);
  }

  /// @brief Stop software timer. After user call Stop(), scheduler don't call
  /// Run() until user calls Start().
  /// @param[in] max_block_time: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @param[in] is_isr: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @return True if timer successfully stopped, false in otherwise.
  auto Stop(paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> ISRbool {
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    ISRbool is_timer_stopped{false};

#ifdef __linux__
    const struct itimerspec itval{};

    if (timer_settime(timer_id_, 0, &itval, nullptr) == 0) {
      is_timer_stopped.SetSuccessStatus(true);
    }
#elif defined(__APPLE__)
    pthread_mutex_lock(&mutex_);
    is_stop_requested_ = true;
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&mutex_);

    if (thread_ != nullptr) {
      pthread_join(thread_, nullptr);
      thread_ = nullptr;
    }

    is_running_ = false;
    is_stop_requested_ = false;
    is_timer_stopped.SetSuccessStatus(true);
#else
#error "Unsupported Unix-like platform"
#endif

    return is_timer_stopped;
  }

  /// @brief Reset software timer. After Reset() called, delay befor next call
  /// Run() method will recalculate relative current moment of the time. If
  /// timer was stopped, calls Run() method will scheduling with <period_ms> and
  /// <is_auto_reload> respect.
  /// @param[in] max_block_time: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @param[in] is_isr: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @return Return true if timer successfully reset, false in otherwise.
  auto Reset(paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> ISRbool {
    return Start(max_block_time, is_isr);
  }

  /// @brief Method called periodical in software timer context with respect
  /// timer creation parameters.
  virtual void Run() {
    // User code must override this method in derivate class.
    PARAOS_CHECK_ASSERT(false);
  }

  /// @brief Five rule.
  Timer(Timer &&other) = delete;
  auto operator=(Timer &&other) -> Timer & = delete;
  auto operator=(const Timer &other) -> Timer & = delete;
  Timer(const Timer &other) = delete;

 private:
  auto Create() -> bool {
    bool is_timer_created{false};

#ifdef __linux__
    struct sigevent sev{};

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_value.sival_ptr = static_cast<void *>(this);
    sev.sigev_notify_function = &Hndlr;
    sev.sigev_notify_attributes = nullptr;
    auto status = timer_create(CLOCK_REALTIME, &sev, &timer_id_);

    if (status == 0) {
      is_timer_created = true;
    }
#elif defined(__APPLE__)
    if (pthread_mutex_init(&mutex_, nullptr) == 0) {
      if (pthread_cond_init(&cond_, nullptr) == 0) {
        is_timer_created = true;
      } else {
        pthread_mutex_destroy(&mutex_);
      }
    }
#else
#error "Unsupported Unix-like platform"
#endif

    return is_timer_created;
  }

#ifdef __linux__
  static void Hndlr(union sigval sigev_value) {
    auto *this_ptr = static_cast<Timer *>(sigev_value.sival_ptr);

    this_ptr->Run();
  }
#elif defined(__APPLE__)
  static auto ThreadRoutine(void *arg) -> void * {
    auto *this_ptr = static_cast<Timer *>(arg);
    this_ptr->RunTimerLoop();
    return nullptr;
  }

  void RunTimerLoop() {
    pthread_mutex_lock(&mutex_);
    while (is_running_ && !is_stop_requested_) {
      struct timespec deadline {};
      struct timespec now {};
      clock_gettime(CLOCK_REALTIME, &now);
      const auto delay = MillisecondsInTimeSpec(period_ms_);
      TimespecAdd(&now, &delay, &deadline);

      int wait_result = 0;
      while (is_running_ && !is_stop_requested_ && wait_result != ETIMEDOUT) {
        wait_result = pthread_cond_timedwait(&cond_, &mutex_, &deadline);
      }

      if (!is_running_ || is_stop_requested_) {
        break;
      }

      if (!is_auto_reload_) {
        is_running_ = false;
      }

      pthread_mutex_unlock(&mutex_);
      Run();
      pthread_mutex_lock(&mutex_);
    }
    pthread_mutex_unlock(&mutex_);
  }
#endif

 private:
  /// @brief Period between scheduler will call Run() method if is_auto_reload_
  /// == true. In otherwise it's delay befor Run() method will called after
  /// user code call Start(). If user set start_immediately == true in ctor,
  /// period_ms_ provide delay befor Run() method will called after software
  /// timer object will constructed.
  std::size_t period_ms_;

  /// @brief If set true, Run() will periodically calls with period_ms_ respect.
  /// In otherwise Run() will called only once with delay, provided by
  /// period_ms_ after user call Start() (or after software timer object will
  /// construct if <start_immediately == true>).
  bool is_auto_reload_;

  std::string_view name_;

#ifdef __linux__
  timer_t timer_id_{std::numeric_limits<timer_t>::max()};
#elif defined(__APPLE__)
  pthread_t thread_{};
  pthread_mutex_t mutex_{};
  pthread_cond_t cond_{};
  bool is_running_{false};
  bool is_stop_requested_{false};
#else
#error "Unsupported Unix-like platform"
#endif
};
}  // namespace paraos

#endif /* PARAOS_TIMER_HPP */
