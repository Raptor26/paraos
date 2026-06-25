/// @file paraos_timer.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_TIMER_HPP
#define PARAOS_TIMER_HPP

#include <cstddef>
#include <limits>
#include <optional>
#include <string_view>

#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_isr.hpp"
#include "paraos_jthread.hpp"
#include "paraos_mutex_std.hpp"
#include "paraos_semaphore_std.hpp"
#include "paraos_sleep.hpp"
#include "paraos_utils.hpp"

namespace paraos {

/// @brief Class provided software timers. For creating timer, user code must
/// provide custom class as derived from paraos::timer.
///
/// @example See example of usages software timers in
/// <example_paraos_timer.cpp>.
///
class timer {
 public:
  /// @brief Software timer ctor.
  /// @param[in] period_ms: Period in microseconds between timer scheduler
  /// calling overriden by user method run().
  /// @param[in] start_immediately: If set true, user code don't need call
  /// start() for start timer. In otherwise, user must call start() for run
  /// timer.
  /// @param[in] is_auto_reload: If set true, overriden by user method run()
  /// will call periodical with period_ms respect. If set false, run() will call
  /// at ones after period_ms delay. If is_auto_reload == false and user code
  /// needs call run() again, call start().
  /// @param[in] name: Human readable string. Useful for debug.
  explicit timer(
      std::size_t period_ms, bool start_immediately = false,
      bool is_auto_reload = true, std::string_view name = "Timer")
      : period_ms_{period_ms}, is_auto_reload_{is_auto_reload}, name_{name} {
    if (start_immediately) {
      start();
    }
  }

  virtual ~timer() { stop(); }

  /// @brief Start timer. If is_auto_reload was set in Ctor, then run() method
  /// will call only once after <period_ms> delay. In other case, run() will
  /// called periodically with <period_ms> delay respect. For change period
  /// use change_period().
  /// @param[in] max_block_time: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @param[in] is_isr: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @return Return true is timer successfully started, false in otherwise.
  auto start(paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> isr_bool {
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    PARAOS_ATTR_UNUSED_VAR(period_ms_);

    isr_bool is_timer_started{true};
    return is_timer_started;
  }

  /// @brief Change period between periodically call run() if timer mode
  /// periodical (is_auto_reload == true), or changed delay before scheduler
  /// call run() after user call start() if one shot timer mode (is_auto_reload
  /// == false).
  /// @param[in] period_ms: New value for period update.
  /// @param[in] max_block_time: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @param[in] is_isr: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @return Return true if period update successfully, false in otherwise.
  auto change_period(
      std::size_t period_ms, paraos::delay_type max_block_time = max_delay,
      bool is_isr = false) -> isr_bool {
    period_ms_ = period_ms;
    return start(max_block_time, is_isr);
  }

  /// @brief Stop software timer. After user call stop(), scheduler don't call
  /// run() until user calls start().
  /// @param[in] max_block_time: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @param[in] is_isr: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @return True if timer successfully stopped, false in otherwise.
  auto stop(paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> isr_bool {
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    PARAOS_ATTR_UNUSED_VAR(period_ms_);

    isr_bool is_timer_stopped{true};
    return is_timer_stopped;
  }

  /// @brief Reset software timer. After reset() called, delay befor next call
  /// run() method will recalculate relative current moment of the time. If
  /// timer was stopped, calls run() method will scheduling with <period_ms> and
  /// <is_auto_reload> respect.
  /// @param[in] max_block_time: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @param[in] is_isr: Backward comptability for FreeRTOS API. Don't
  /// used in Unix.
  /// @return Return true if timer successfully reset, false in otherwise.
  auto reset(paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> isr_bool {
    return start(max_block_time, is_isr);
  }

  /// @brief Method called periodical in software timer context with respect
  /// timer creation parameters.
  virtual void run() {
    // User code must override this method in derivate class.
    PARAOS_CHECK_ASSERT(false);
  }

  /// @brief Five rule.
  timer(timer&& other) = delete;
  auto operator=(timer&& other) -> timer& = delete;
  auto operator=(const timer& other) -> timer& = delete;
  timer(const timer& other) = delete;

  [[nodiscard]] PARAOS_DEPRECATED("use start()") auto Start(
      paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> isr_bool {
    return start(max_block_time, is_isr);
  }

  [[nodiscard]] PARAOS_DEPRECATED("use change_period()") auto ChangePeriod(
      std::size_t period_ms, paraos::delay_type max_block_time = max_delay,
      bool is_isr = false) -> isr_bool {
    return change_period(period_ms, max_block_time, is_isr);
  }

  [[nodiscard]] PARAOS_DEPRECATED("use stop()") auto Stop(
      paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> isr_bool {
    return stop(max_block_time, is_isr);
  }

  [[nodiscard]] PARAOS_DEPRECATED("use reset()") auto Reset(
      paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> isr_bool {
    return reset(max_block_time, is_isr);
  }

 private:
  /// @brief Period between scheduler will call run() method if is_auto_reload_
  /// == true. In otherwise it's delay befor run() method will called after
  /// user code call start(). If user set start_immediately == true in ctor,
  /// period_ms_ provide delay befor run() method will called after software
  /// timer object will constructed.
  std::size_t period_ms_;

  /// @brief If set true, run() will periodically calls with period_ms_ respect.
  /// In otherwise run() will called only once with delay, provided by
  /// period_ms_ after user call start() (or after software timer object will
  /// construct if <start_immediately == true>).
  [[maybe_unused]] bool is_auto_reload_;

  std::string_view name_;

  paraos::mutex mutex_;
  paraos::binary_semaphore wake_sem_{0};
  std::optional<paraos::jthread> worker_;
  [[maybe_unused]] bool is_running_{false};
  [[maybe_unused]] bool is_stop_requested_{false};
};

using Timer PARAOS_DEPRECATED("use paraos::timer") = timer;

}  // namespace paraos

#endif /* PARAOS_TIMER_HPP */
