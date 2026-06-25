/// @file paraos_timer.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_TIMER_HPP
#define PARAOS_TIMER_HPP

#include <etl/error_handler.h>

#include <new>
#include <string_view>

#include "FreeRTOS.h"
#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_isr.hpp"
#include "paraos_utils.hpp"
#include "timers.h"

namespace paraos {

/// @brief Class provided software timers. For creating timer, user code must
/// provide custom class as derived from paraos::timer.
///
/// @see example_paraos_timer.cpp for more information about using 'timer'
/// class.
class timer {
 public:
  /// @brief Software timer ctor. Create timer and start execute immediately (if
  /// needed).
  /// @param[in] period_ms: Period in miliseconds for calling run() method,
  /// which user code must override in custom class.
  explicit timer(
      std::size_t period_ms, bool start_immediately = false,
      bool is_auto_reload = true, std::string_view name = "Timer") {
    handle_ = xTimerCreate(
        name.data(), PARAOS_ConvertMsToTicks(period_ms),
        static_cast<BaseType_t>(is_auto_reload), static_cast<void*>(this),
        timer_callback);

    ETL_ASSERT(handle_ != nullptr, std::bad_alloc());

    if (start_immediately) {
      start();
    }
  }

  /// @brief Start software timer.
  ///
  /// @param[in] max_block_time_ms: FreeRTOS can wait period, specified in
  /// max_block_time_ms if no space in queue for registered start timer command.
  /// @param[in] is_isr: Set true if API called from ISR.
  ///
  /// @return Return true if command successfully pushed in timer queue.
  /// @note If is_isr == true, return value contained field, specified is need
  /// switch RTOS context from ISR.
  auto start(
      paraos::delay_type max_block_time_ms = max_delay, bool is_isr = false)
      -> isr_bool {
    isr_bool is_timer_started;
    BaseType_t xHigherPriorityTaskWoken{pdFALSE};
    if (handle_ != nullptr) {
      if (!is_isr) {
        is_timer_started.set_success_status(
            static_cast<bool>(xTimerStart(
                handle_, PARAOS_ConvertMsToTicks(max_block_time_ms))));

      } else {
        is_timer_started.set_success_status(
            static_cast<bool>(
                xTimerStartFromISR(handle_, &xHigherPriorityTaskWoken)));

        if (xHigherPriorityTaskWoken != pdFALSE) {
          is_timer_started.set_switch_context_status(true);
        }
      }
    }
    return is_timer_started;
  }

  /// @brief Change period between scheduler call callback function.
  ///
  /// @param[in] period_ms; New period between scheduler call callback function.
  /// @param[in] max_block_time_ms; FreeRTOS can wait period, specified in
  /// max_block_time_ms if no space in queue for registered change_period
  /// command.
  /// @param[in] is_isr: Set true if API called from ISR.
  ///
  /// @return Return true if command successfully pushed in timer queue.
  /// @note If is_isr == true, return value contained field, specified is need
  /// switch RTOS context from ISR.
  auto change_period(
      std::size_t period_ms, paraos::delay_type max_block_time_ms = max_delay,
      bool is_isr = false) noexcept {
    isr_bool is_period_changed;
    BaseType_t xHigherPriorityTaskWoken{pdFALSE};
    if (!is_isr) {
      is_period_changed.set_success_status(
          static_cast<bool>(xTimerChangePeriod(
              handle_, period_ms, PARAOS_ConvertMsToTicks(max_block_time_ms))));
    } else {
      is_period_changed.set_success_status(
          static_cast<bool>(xTimerChangePeriodFromISR(
              handle_, period_ms, &xHigherPriorityTaskWoken)));

      if (xHigherPriorityTaskWoken != pdFALSE) {
        is_period_changed.set_switch_context_status(true);
      }
    }

    return is_period_changed;
  }

  /// @brief Stop scheduling callback function.
  ///
  /// @param[in] max_block_time_ms; FreeRTOS can wait period, specified in
  /// max_block_time_ms if no space in queue for registered stop command.
  /// @param[in] is_isr: Set true if API called from ISR.
  ///
  /// @return Return true if command successfully pushed in timer queue.
  /// @note If is_isr == true, return value contained field, specified is need
  /// switch RTOS context from ISR.
  auto stop(
      paraos::delay_type max_block_time_ms = max_delay, bool is_isr = false) {
    isr_bool is_stopped;
    BaseType_t xHigherPriorityTaskWoken{pdFALSE};
    if (!is_isr) {
      is_stopped.set_success_status(
          static_cast<bool>(
              xTimerStop(handle_, PARAOS_ConvertMsToTicks(max_block_time_ms))));
    } else {
      is_stopped.set_success_status(
          static_cast<bool>(
              xTimerStopFromISR(handle_, &xHigherPriorityTaskWoken)));

      if (xHigherPriorityTaskWoken != pdFALSE) {
        is_stopped.set_switch_context_status(true);
      }
    }

    return is_stopped;
  }

  /// @brief Re-starts a timer that was previously created using the ctor. If
  /// the timer had already been started and was already in the active state,
  /// then reset() will cause the timer to re- evaluate its expiry time so
  /// that it is relative to when reset() was called. If the timer was in
  /// the dormant state then reset() has equivalent functionality to the
  /// reset() API function.
  ///
  /// @details Resetting a timer ensures the timer is in the active state. If
  /// the timer is not stopped, deleted, or reset in the mean time, the callback
  /// function associated with the timer will get called 'n' ticks after reset()
  /// was called, where 'n' is the timers defined period.
  ///
  /// @param[in] max_block_time_ms; FreeRTOS can wait period, specified in
  /// max_block_time_ms if no space in queue for registered stop command.
  /// @param[in] is_isr: Set true if API called from ISR.
  ///
  /// @return Return true if command successfully pushed in timer queue.
  /// @note If is_isr == true, return value contained field, specified is need
  /// switch RTOS context from ISR.
  auto reset(
      paraos::delay_type max_block_time_ms = max_delay,
      bool is_isr = false) noexcept {
    // reset not provided ISR API.
    PARAOS_CHECK_ASSERT(is_isr == false);
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    isr_bool is_reset;

    is_reset.set_success_status(
        static_cast<bool>(
            xTimerReset(handle_, PARAOS_ConvertMsToTicks(max_block_time_ms))));

    return is_reset;
  }

  virtual ~timer() {
    xTimerDelete(handle_, PARAOS_ConvertMsToTicks(max_delay));
  }

  virtual void run() = 0;

  /// @brief Five rule.
  timer(timer&& other) = delete;
  auto operator=(timer&& other) -> timer& = delete;
  auto operator=(const timer& other) -> timer& = delete;
  timer(const timer& other) = delete;

  [[nodiscard]] PARAOS_DEPRECATED("use start()") auto Start(
      paraos::delay_type max_block_time_ms = max_delay, bool is_isr = false)
      -> isr_bool {
    return start(max_block_time_ms, is_isr);
  }

  [[nodiscard]] PARAOS_DEPRECATED("use change_period()") auto ChangePeriod(
      std::size_t period_ms, paraos::delay_type max_block_time_ms = max_delay,
      bool is_isr = false) noexcept {
    return change_period(period_ms, max_block_time_ms, is_isr);
  }

  [[nodiscard]] PARAOS_DEPRECATED("use stop()") auto Stop(
      paraos::delay_type max_block_time_ms = max_delay, bool is_isr = false) {
    return stop(max_block_time_ms, is_isr);
  }

  [[nodiscard]] PARAOS_DEPRECATED("use reset()") auto Reset(
      paraos::delay_type max_block_time_ms = max_delay,
      bool is_isr = false) noexcept {
    return reset(max_block_time_ms, is_isr);
  }

 private:
  static void timer_callback(TimerHandle_t timer_handle) {
    PARAOS_CHECK_ASSERT(timer_handle);
    auto* this_ptr = reinterpret_cast<timer*>(pvTimerGetTimerID(timer_handle));

    this_ptr->run();
  }

 private:
  TimerHandle_t handle_;
};

using Timer PARAOS_DEPRECATED("use paraos::timer") = timer;

}  // namespace paraos

#endif /* PARAOS_TIMER_HPP */
