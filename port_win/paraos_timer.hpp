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

namespace paraos {

/// @brief Class provided software timers. For creating timer, user code must
/// provide custom class as derived from paraos::Timer.
///
/// @see example_paraos_timer.cpp for more information about using 'Timer'
/// class.
class Timer {
  /// @brief  After timer created, his execute will be scheduling immediately.
  /// @return Return true if timer started, false in otherwise.
  auto Create() {
    bool is_timer_created{false};
    if (!timer_) {
      // Registered CreateTimerQueueTimer() callback function in default timer
      // queue.
      is_timer_created = CreateTimerQueueTimer(
          &timer_, nullptr, WaitOrTimerCallback, static_cast<PVOID>(this), 0u,
          period_ms_, WT_EXECUTEINTIMERTHREAD);
    }

    return is_timer_created;
  }

 public:
  /// @brief Software timer ctor. Create timer and start execute immediately (if
  /// needed).
  /// @param[in] period_ms: Period in miliseconds for calling Run() method,
  /// which user code must override in custom class.
  Timer(std::size_t period_ms, bool start_immediately = false)
      : period_ms_{period_ms} {
    if (start_immediately == true) {
      Create();
    }
  }

  /// @brief Start timer.
  /// @details In windows, timer's execute scheduling immediately. For simulate
  /// deferred timer start, create timer here instead creating in ctor.
  auto Start() {
    // Checking double timer creation run in Create().
    return Create();
  }

  /// @brief Change period for calling Run() method.
  /// @param[in] period_ms: After this period Run() method will be call
  /// periodical.
  /// @return Return true if period successfully changed.
  auto ChangePeriod(std::size_t period_ms) {
    period_ms_ = period_ms;
    return ChangeTimerQueueTimer(nullptr, timer_, period_ms_, period_ms_);
  }

  /// @brief Stop periodical scheduling Run() execute.
  /// @details For simulate stop operation, we delete timer. When user call
  /// Start(), timer will be create again.
  void Stop() { Delete(); }

  virtual ~Timer() { Delete(); }

  /// @brief Method called periodical in software timer context with respect
  /// timer creation parameters.
  virtual void Run() {
    // User code must override this method in derivate class.
    PARAOS_CHECK_ASSERT(false);
  }

 private:
  void Delete() { DeleteTimerQueueTimer(timer_, nullptr, nullptr); }

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
    auto* this_ptr = reinterpret_cast<Timer*>(lpParameter);
    this_ptr->Run();
  }

 private:
  HANDLE timer_{nullptr};
  std::size_t period_ms_;
};

}  // namespace paraos

#endif /* PARAOS_TIMER_HPP */
