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

#include <string_view>

#include "FreeRTOS.h"
#include "paraos_utils.hpp"
#include "paroas_isr.hpp"
#include "timers.h"

namespace paraos {

class Timer {
 public:
  Timer(
      std::size_t period_ms, bool start_immediately = false,
      bool is_auto_reload = true, std::string_view name = "Timer") {
    handle_ = xTimerCreate(
        name.data(), PARAOS_ConvertMsToTicks(period_ms), is_auto_reload,
        static_cast<void*>(this), TimerCallback);

    if (start_immediately) {
      Start();
    }
  }

  ISRbool Start(std::size_t max_block_time = max_delay, bool is_isr = false) {
    ISRbool is_timer_started;
    BaseType_t xHigherPriorityTaskWoken{pdFALSE};
    if (handle_) {
      if (!is_isr) {
        is_timer_started.is_success_ =
            xTimerStart(handle_, PARAOS_ConvertMsToTicks(max_block_time));
      } else {
        is_timer_started.is_success_ =
            xTimerStartFromISR(handle_, &xHigherPriorityTaskWoken);

        if (xHigherPriorityTaskWoken != pdFALSE) {
          is_timer_started.is_need_switch_context_ = true;
        }
      }
    }
    return is_timer_started;
  }

  ISRbool ChangePeriod(
      std::size_t period_ms, std::size_t max_block_time = max_delay,
      bool is_isr = false) {
    ISRbool is_period_changed;
    BaseType_t xHigherPriorityTaskWoken{pdFALSE};
    if (!is_isr) {
      is_period_changed.is_success_ = xTimerChangePeriod(
          handle_, period_ms, PARAOS_ConvertMsToTicks(max_block_time));
    } else {
      is_period_changed.is_success_ = xTimerChangePeriodFromISR(
          handle_, period_ms, &xHigherPriorityTaskWoken);

      if (xHigherPriorityTaskWoken != pdFALSE) {
        is_period_changed.is_need_switch_context_ = true;
      }
    }

    return is_period_changed;
  }

  ISRbool Stop(std::size_t max_block_time = max_delay, bool is_isr = false) {
    ISRbool is_stopped;
    BaseType_t xHigherPriorityTaskWoken{pdFALSE};
    if (!is_isr) {
      is_stopped.is_success_ =
          xTimerStop(handle_, PARAOS_ConvertMsToTicks(max_block_time));
    } else {
      is_stopped.is_success_ =
          xTimerStopFromISR(handle_, &xHigherPriorityTaskWoken);

      if (xHigherPriorityTaskWoken != pdFALSE) {
        is_stopped.is_need_switch_context_ = true;
      }
    }

    return is_stopped;
  }

  ISRbool Reset(std::size_t max_block_time = max_delay, bool is_isr = false) {
    // Reset not provided ISR API.
    PARAOS_CHECK_ASSERT(is_isr == false);

    ISRbool is_reset;
    BaseType_t xHigherPriorityTaskWoken{pdFALSE};

    is_reset.is_success_ =
        xTimerReset(handle_, PARAOS_ConvertMsToTicks(max_block_time));

    return is_reset;
  }

  virtual ~Timer() {
    xTimerDelete(handle_, PARAOS_ConvertMsToTicks(max_delay));
  }

  virtual void Run() = 0;

 private:
  static void TimerCallback(TimerHandle_t timer_handle) {
    PARAOS_CHECK_ASSERT(timer_handle);
    auto this_ptr = reinterpret_cast<Timer*>(pvTimerGetTimerID(timer_handle));

    this_ptr->Run();
  }

 private:
  TimerHandle_t handle_;
};

}  // namespace paraos

#endif /* PARAOS_TIMER_HPP */
