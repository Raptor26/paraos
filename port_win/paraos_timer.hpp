/// @file paraos_timer.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @see https://www.codeproject.com/Articles/1236/Timers-Tutorial
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

#include <Windows.h>
#include <threadpoollegacyapiset.h>

#include "etl/delegate.h"
#include "paraos_check.h"
#include "paroas_isr.hpp"

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
  Timer(
      std::size_t period_ms, bool start_immediately = false,
      bool is_auto_reload = true, std::string_view name = "Timer")
      : period_ms_{period_ms}, is_auto_reload_{is_auto_reload}, name_{name} {
    if (start_immediately == true) {
      Create();
    }
  }

  /// @brief Start timer.
  /// @details In windows, timer's execute scheduling immediately. For simulate
  /// deferred timer start, create timer here instead creating in ctor.
  auto Start(std::size_t max_block_time = max_delay, bool is_isr = false)
      -> ISRbool {
    // PARAOS wrapper for winapi not support isr context.
    PARAOS_CHECK_ASSERT(is_isr != true);

    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    // Checking double timer creation run in Create().
    return Create();
  }

  /// @brief Change period for calling Run() method.
  /// @param[in] period_ms: After this period Run() method will be call
  /// periodical.
  /// @return Return true if period successfully changed.
  auto ChangePeriod(
      std::size_t period_ms, std::size_t max_block_time = max_delay,
      bool is_isr = false) -> ISRbool {
    // PARAOS wrapper for winapi not support isr context.
    PARAOS_CHECK_ASSERT(is_isr != true);
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    period_ms_ = period_ms;
    return ChangeTimerQueueTimer(nullptr, timer_, period_ms_, period_ms_);
  }

  /// @brief Stop periodical scheduling Run() execute.
  /// @details For simulate stop operation, we delete timer. When user call
  /// Start(), timer will be create again.
  auto Stop(std::size_t max_block_time = max_delay, bool is_isr = false)
      -> ISRbool {
    // PARAOS wrapper for winapi not support isr context.
    PARAOS_CHECK_ASSERT(is_isr != true);
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    return Delete();
  }

  /// @brief Re-start timer. If timer already started, Reset() will cause the
  /// timer to re-evaluate its expiry time so that it is relative to when
  /// Reset() was called.
  /// @param[in] max_block_time_ms: In winapi is fake parameter, which needed
  /// for compatibility for freeRTOS API.
  /// @param[in] is_isr: In winapi is fake parameter, which needed
  /// for compatibility for freeRTOS API.
  /// @return True is timer successfully restarted, false on otherwise.
  ISRbool Reset(
      std::size_t max_block_time_ms = max_delay, bool is_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(max_block_time_ms);
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    Delete();
    return Start(max_delay, is_isr);
  }

  virtual ~Timer() { Delete(); }

  /// @brief Method called periodical in software timer context with respect
  /// timer creation parameters.
  virtual void Run() {
    // User code must override this method in derivate class.
    PARAOS_CHECK_ASSERT(false);
  }

 private:
  /// @brief  After timer created, his execute will be scheduling immediately.
  /// @return Return true if timer started, false in otherwise.
  auto Create() -> bool {
    bool is_timer_created{false};
    if (!timer_) {
      // Registered CreateTimerQueueTimer() callback function in default timer
      // queue.

      auto period_ms{period_ms_};
      if (!is_auto_reload_) {
        // We set period_ms as zero for one shot timer execute.
        period_ms = 0u;
      }

      is_timer_created = CreateTimerQueueTimer(
          &timer_, nullptr, WaitOrTimerCallback, static_cast<PVOID>(this),
          static_cast<DWORD>(
              period_ms_),  // time befor fist call callback function
          static_cast<DWORD>(
              period_ms),  // in one shot mode, this will be zero value

          WT_EXECUTEINTIMERTHREAD);
    }

    return is_timer_created;
  }

  auto Delete() -> ISRbool {
    auto is_timer_deleted = DeleteTimerQueueTimer(nullptr, timer_, nullptr);
    timer_ = nullptr;

    return is_timer_deleted != 0 ? true : false;
  }

  /// @brief Starting address for a timer callback or a registered wait
  /// callback.
  ///
  /// @see
  /// https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms687066(v=vs.85)
  ///
  /// @param[in] lpParameter: The thread data passed to the function using a
  /// parameter of the CreateTimerQueueTimer or RegisterWaitForSingleObject
  /// function.
  /// @param[in] TimerOrWaitFired: If this parameter is TRUE, the wait timed
  /// out. If this parameter is FALSE, the wait event has been signaled. (This
  /// parameter is always TRUE for timer callbacks.)
  ///
  /// @return None
  static VOID CALLBACK
  WaitOrTimerCallback(_In_ PVOID lpParameter, _In_ BOOLEAN TimerOrWaitFired) {
    PARAOS_CHECK_ASSERT(lpParameter);
    PARAOS_CHECK_ASSERT(TimerOrWaitFired == true);
    PARAOS_ATTR_UNUSED_VAR(TimerOrWaitFired);
    auto* this_ptr = reinterpret_cast<Timer*>(lpParameter);
    this_ptr->Run();
  }

 private:
  HANDLE timer_{nullptr};

  /// @brief Period befor periodically call callback function, but if
  /// is_auto_reload_ == false, it's period befor first and only one call
  /// callback function.
  std::size_t period_ms_;
  bool is_auto_reload_;
  std::string_view name_;
};

}  // namespace paraos

#endif /* PARAOS_TIMER_HPP */
