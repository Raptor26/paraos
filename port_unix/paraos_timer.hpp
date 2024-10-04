

#ifndef PARAOS_TIMER_HPP
#define PARAOS_TIMER_HPP

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <cstring>
#include <string_view>

#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_utils.hpp"
#include "paroas_isr.hpp"

namespace paraos {

/// @brief
/// @see
/// https://stackoverflow.com/questions/64429205/how-to-use-sigev-thread-sigevent-for-linux-timers-expiration-handling-in-c
class Timer {
 public:
  Timer(
      std::size_t period_ms, bool start_immediately = false,
      bool is_auto_reload = true, std::string_view name = "Timer")
      : period_ms_{period_ms}, is_auto_reload_{is_auto_reload}, name_{name} {
    Create();
    if (start_immediately == true) {
      Start();
    }
  }

  virtual ~Timer() {
    auto status = timer_delete(timerid_);
    PARAOS_CHECK_ASSERT(status == 0);
    PARAOS_ATTR_UNUSED_VAR(status);
  }

  auto Start(std::size_t max_block_time = max_delay, bool is_isr = false)
      -> ISRbool {
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    struct itimerspec itval {};
    ISRbool is_timer_started{false};

    if (is_auto_reload_) {
      itval.it_value = MilisecondsInTimerSpec(period_ms_);
      itval.it_interval.tv_sec = itval.it_value.tv_sec;
      itval.it_interval.tv_nsec = itval.it_value.tv_nsec;
    } else {
      itval.it_value.tv_nsec = 1u;
    }

    auto status = timer_settime(timerid_, 0, &itval, nullptr);
    if (status == 0) {
      is_timer_started.is_success_ = true;
    }

    return is_timer_started;
  }

  auto ChangePeriod(
      std::size_t period_ms, std::size_t max_block_time = max_delay,
      bool is_isr = false) -> ISRbool {
    ISRbool is_period_changed{false};
    period_ms_ = period_ms;

    return Start(max_block_time, is_isr);
  }

  auto Stop(std::size_t max_block_time = max_delay, bool is_isr = false)
      -> ISRbool {
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    ISRbool is_timer_stopped{false};

    struct itimerspec itval {};

    if (timer_settime(timerid_, 0, &itval, nullptr) == 0) {
      is_timer_stopped.is_success_ = true;
    }

    return is_timer_stopped;
  }

  auto Reset(std::size_t max_block_time = max_delay, bool is_isr = false)
      -> ISRbool {
    return Start(max_block_time, is_isr);
  }

  /// @brief Method called periodical in software timer context with respect
  /// timer creation parameters.
  virtual void Run() {
    // User code must override this method in derivate class.
    PARAOS_CHECK_ASSERT(false);
  }

 private:
  bool Create() {
    bool is_timer_created{false};

    struct sigevent sev {};

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_value.sival_ptr = static_cast<void *>(this);
    sev.sigev_notify_function = &Hndlr;
    sev.sigev_notify_attributes = 0;
    auto status = timer_create(CLOCK_REALTIME, &sev, &timerid_);

    if (status == 0) {
      is_timer_created = true;
    }

    return is_timer_created;
  }

  static void Hndlr(union sigval sigev_value) {
    auto this_ptr = static_cast<Timer *>(sigev_value.sival_ptr);

    this_ptr->Run();
  }

 private:
  std::size_t period_ms_;
  bool is_auto_reload_;
  std::string_view name_;
  timer_t timerid_{0};
};
}  // namespace paraos

#endif /* PARAOS_TIMER_HPP */
