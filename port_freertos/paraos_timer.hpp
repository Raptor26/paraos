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
/// provide custom class as derived from paraos::Timer.
///
/// @see example_paraos_timer.cpp for more information about using 'Timer'
/// class.
class Timer {
 public:
  /// @brief Software timer ctor. Create timer and start execute immediately (if
  /// needed).
  /// @param[in] period_ms: Period in miliseconds for calling Run() method,
  /// which user code must override in custom class.
  explicit Timer(
      std::size_t period_ms, bool start_immediately = false,
      bool is_auto_reload = true, std::string_view name = "Timer") {
    handle_ = xTimerCreate(
        name.data(), PARAOS_ConvertMsToTicks(period_ms),
        static_cast<BaseType_t>(is_auto_reload), static_cast<void *>(this),
        TimerCallback);

    ETL_ASSERT(handle_ != nullptr, std::bad_alloc());

    if (start_immediately) {
      Start();
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
  auto Start(
      paraos::delay_type max_block_time_ms = max_delay, bool is_isr = false)
      -> ISRbool {
    ISRbool is_timer_started;
    BaseType_t xHigherPriorityTaskWoken{pdFALSE};
    if (handle_ != nullptr) {
      if (!is_isr) {
        is_timer_started.SetSuccessStatus(
            static_cast<bool>(xTimerStart(
                handle_, PARAOS_ConvertMsToTicks(max_block_time_ms))));

      } else {
        is_timer_started.SetSuccessStatus(
            static_cast<bool>(
                xTimerStartFromISR(handle_, &xHigherPriorityTaskWoken)));

        if (xHigherPriorityTaskWoken != pdFALSE) {
          is_timer_started.SetSwitchContextStatus(true);
        }
      }
    }
    return is_timer_started;
  }

  /// @brief Change period between scheduler call callback function.
  ///
  /// @param[in] period_ms; New period between scheduler call callback function.
  /// @param[in] max_block_time_ms; FreeRTOS can wait period, specified in
  /// max_block_time_ms if no space in queue for registered ChangePeriod
  /// command.
  /// @param[in] is_isr: Set true if API called from ISR.
  ///
  /// @return Return true if command successfully pushed in timer queue.
  /// @note If is_isr == true, return value contained field, specified is need
  /// switch RTOS context from ISR.
  auto ChangePeriod(
      std::size_t period_ms, paraos::delay_type max_block_time_ms = max_delay,
      bool is_isr = false) noexcept {
    ISRbool is_period_changed;
    BaseType_t xHigherPriorityTaskWoken{pdFALSE};
    if (!is_isr) {
      is_period_changed.SetSuccessStatus(
          static_cast<bool>(xTimerChangePeriod(
              handle_, period_ms, PARAOS_ConvertMsToTicks(max_block_time_ms))));
    } else {
      is_period_changed.SetSuccessStatus(
          static_cast<bool>(xTimerChangePeriodFromISR(
              handle_, period_ms, &xHigherPriorityTaskWoken)));

      if (xHigherPriorityTaskWoken != pdFALSE) {
        is_period_changed.SetSwitchContextStatus(true);
      }
    }

    return is_period_changed;
  }

  /// @brief Stop scheduling callback function.
  ///
  /// @param[in] max_block_time_ms; FreeRTOS can wait period, specified in
  /// max_block_time_ms if no space in queue for registered Stop command.
  /// @param[in] is_isr: Set true if API called from ISR.
  ///
  /// @return Return true if command successfully pushed in timer queue.
  /// @note If is_isr == true, return value contained field, specified is need
  /// switch RTOS context from ISR.
  auto Stop(
      paraos::delay_type max_block_time_ms = max_delay, bool is_isr = false) {
    ISRbool is_stopped;
    BaseType_t xHigherPriorityTaskWoken{pdFALSE};
    if (!is_isr) {
      is_stopped.SetSuccessStatus(
          static_cast<bool>(
              xTimerStop(handle_, PARAOS_ConvertMsToTicks(max_block_time_ms))));
    } else {
      is_stopped.SetSuccessStatus(
          static_cast<bool>(
              xTimerStopFromISR(handle_, &xHigherPriorityTaskWoken)));

      if (xHigherPriorityTaskWoken != pdFALSE) {
        is_stopped.SetSwitchContextStatus(true);
      }
    }

    return is_stopped;
  }

  /// @brief Re-starts a timer that was previously created using the ctor. If
  /// the timer had already been started and was already in the active state,
  /// then Reset() will cause the timer to re- evaluate its expiry time so
  /// that it is relative to when Reset() was called. If the timer was in
  /// the dormant state then Reset() has equivalent functionality to the
  /// Reset() API function.
  ///
  /// @details Resetting a timer ensures the timer is in the active state. If
  /// the timer is not stopped, deleted, or reset in the mean time, the callback
  /// function associated with the timer will get called 'n' ticks after Reset()
  /// was called, where 'n' is the timers defined period.
  ///
  /// @param[in] max_block_time_ms; FreeRTOS can wait period, specified in
  /// max_block_time_ms if no space in queue for registered Stop command.
  /// @param[in] is_isr: Set true if API called from ISR.
  ///
  /// @return Return true if command successfully pushed in timer queue.
  /// @note If is_isr == true, return value contained field, specified is need
  /// switch RTOS context from ISR.
  auto Reset(
      paraos::delay_type max_block_time_ms = max_delay,
      bool is_isr = false) noexcept {
    // Reset not provided ISR API.
    PARAOS_CHECK_ASSERT(is_isr == false);
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    ISRbool is_reset;

    is_reset.SetSuccessStatus(
        static_cast<bool>(
            xTimerReset(handle_, PARAOS_ConvertMsToTicks(max_block_time_ms))));

    return is_reset;
  }

  virtual ~Timer() {
    xTimerDelete(handle_, PARAOS_ConvertMsToTicks(max_delay));
  }

  virtual void Run() = 0;

  /// @brief Five rule.
  Timer(Timer &&other) = delete;
  auto operator=(Timer &&other) -> Timer & = delete;
  auto operator=(const Timer &other) -> Timer & = delete;
  Timer(const Timer &other) = delete;

 private:
  static void TimerCallback(TimerHandle_t timer_handle) {
    PARAOS_CHECK_ASSERT(timer_handle);
    auto *this_ptr = reinterpret_cast<Timer *>(pvTimerGetTimerID(timer_handle));

    this_ptr->Run();
  }

 private:
  TimerHandle_t handle_;
};

}  // namespace paraos

#endif /* PARAOS_TIMER_HPP */
