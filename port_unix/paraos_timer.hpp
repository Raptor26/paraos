/// @file paraos_timer.hpp
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

#ifndef PARAOS_TIMER_HPP
#define PARAOS_TIMER_HPP

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
#include "paraos_utils.hpp"
#include "paroas_isr.hpp"

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
    auto status = timer_delete(timer_id_);
    PARAOS_CHECK_ASSERT(status == 0);
    PARAOS_ATTR_UNUSED_VAR(status);
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
  auto Start(std::size_t max_block_time = max_delay, bool is_isr = false)
      -> ISRbool {
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    struct itimerspec itval {};
    ISRbool is_timer_started{false};

    if (is_auto_reload_) {
      itval.it_value = MillisecondsInTimeSpec(period_ms_);
      itval.it_interval.tv_sec = itval.it_value.tv_sec;
      itval.it_interval.tv_nsec = itval.it_value.tv_nsec;
    } else {
      itval.it_value.tv_nsec = 1u;
    }

    auto status = timer_settime(timer_id_, 0, &itval, nullptr);
    if (status == 0) {
      is_timer_started.is_success_ = true;
    }

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
      std::size_t period_ms, std::size_t max_block_time = max_delay,
      bool is_isr = false) -> ISRbool {
    ISRbool is_period_changed{false};
    period_ms_ = period_ms;

    return Start(max_block_time, is_isr);
  }

  /// @brief Stop software timer. After user call Stop(), scheduler don'tt call
  /// Run() until user calls Start().
  /// @param[in] max_block_time: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @param[in] is_isr: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @return True if timer successfully stopped, false in otherwise.
  auto Stop(std::size_t max_block_time = max_delay, bool is_isr = false)
      -> ISRbool {
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    ISRbool is_timer_stopped{false};

    struct itimerspec itval {};

    if (timer_settime(timer_id_, 0, &itval, nullptr) == 0) {
      is_timer_stopped.is_success_ = true;
    }

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
    auto status = timer_create(CLOCK_REALTIME, &sev, &timer_id_);

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
  /// @brief Period between scheduler will call Run() method if is_auto_reload_
  /// == true. In otherwise, it's delay befor Run() method will called after
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
  timer_t timer_id_{std::numeric_limits<timer_t>::max()};
};
}  // namespace paraos

#endif /* PARAOS_TIMER_HPP */
