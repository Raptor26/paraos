/// @file paraos_timer.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_TIMER_HPP
#define PARAOS_TIMER_HPP

#include <threadpoollegacyapiset.h>

#include "etl/delegate.h"
#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_isr.hpp"
#include "paraos_utils.hpp"

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
      bool is_auto_reload = true, std::string_view name = "Timer")
      : period_ms_{period_ms}, is_auto_reload_{is_auto_reload}, name_{name} {
    if (start_immediately) {
      create();
    }
  }

  /// @brief Start timer.
  /// @details In windows, timer's execute scheduling immediately. For simulate
  /// deferred timer start, create timer here instead creating in ctor.
  auto start(paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> isr_bool {
    // PARAOS wrapper for winapi not support isr context.
    PARAOS_CHECK_ASSERT(is_isr != true);

    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    // Checking double timer creation run in create().
    return static_cast<isr_bool>(create());
  }

  /// @brief Change period for calling run() method.
  /// @param[in] period_ms: After this period run() method will be call
  /// periodical.
  /// @return Return true if period successfully changed.
  auto change_period(
      std::size_t period_ms, std::size_t max_block_time = max_delay,
      bool is_isr = false) -> isr_bool {
    // PARAOS wrapper for winapi not support isr context.
    PARAOS_CHECK_ASSERT(is_isr != true);
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    period_ms_ = period_ms;
    return static_cast<isr_bool>(static_cast<bool>(
        ChangeTimerQueueTimer(nullptr, timer_, period_ms_, period_ms_)));
  }

  /// @brief Stop periodical scheduling run() execute.
  /// @details For simulate stop operation, we delete timer. When user call
  /// start(), timer will be create again.
  auto stop(paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> isr_bool {
    // PARAOS wrapper for winapi not support isr context.
    PARAOS_CHECK_ASSERT(is_isr != true);
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    return Delete();
  }

  /// @brief Re-start timer. If timer already started, reset() will cause the
  /// timer to re-evaluate its expiry time so that it is relative to when
  /// reset() was called.
  /// @param[in] max_block_time_ms: In winapi is fake parameter, which needed
  /// for compatibility for freeRTOS API.
  /// @param[in] is_isr: In winapi is fake parameter, which needed
  /// for compatibility for freeRTOS API.
  /// @return True is timer successfully restarted, false on otherwise.
  auto reset(
      paraos::delay_type max_block_time_ms = max_delay, bool is_isr = false)
      -> isr_bool {
    PARAOS_ATTR_UNUSED_VAR(max_block_time_ms);
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    Delete();
    return start(max_delay, is_isr);
  }

  virtual ~timer() { Delete(); }

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
      std::size_t period_ms, std::size_t max_block_time = max_delay,
      bool is_isr = false) -> isr_bool {
    return change_period(period_ms, max_block_time, is_isr);
  }

  [[nodiscard]] PARAOS_DEPRECATED("use stop()") auto Stop(
      paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> isr_bool {
    return stop(max_block_time, is_isr);
  }

  [[nodiscard]] PARAOS_DEPRECATED("use reset()") auto Reset(
      paraos::delay_type max_block_time_ms = max_delay, bool is_isr = false)
      -> isr_bool {
    return reset(max_block_time_ms, is_isr);
  }

 private:
  /// @brief  After timer created, his execute will be scheduling immediately.
  /// @return Return true if timer started, false in otherwise.
  auto create() -> bool {
    bool is_timer_created{false};
    if (timer_ == nullptr) {
      // Registered CreateTimerQueueTimer() callback function in default timer
      // queue.

      auto period_ms{period_ms_};
      if (!is_auto_reload_) {
        // We set period_ms as zero for one shot timer execute.
        period_ms = 0U;
      }

      is_timer_created = static_cast<bool>(CreateTimerQueueTimer(
          &timer_, nullptr, wait_or_timer_callback, static_cast<PVOID>(this),
          static_cast<DWORD>(
              period_ms_),  // time befor fist call callback function
          static_cast<DWORD>(
              period_ms),  // in one shot mode, this will be zero value

          WT_EXECUTEINTIMERTHREAD));
    }

    return is_timer_created;
  }

  auto Delete() -> isr_bool {
    auto is_timer_deleted = DeleteTimerQueueTimer(nullptr, timer_, nullptr);
    timer_ = nullptr;

    return static_cast<isr_bool>(static_cast<bool>(is_timer_deleted != 0));
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
  static VOID CALLBACK wait_or_timer_callback(
      _In_ PVOID lpParameter, _In_ BOOLEAN TimerOrWaitFired) {
    PARAOS_CHECK_ASSERT(lpParameter);
    PARAOS_CHECK_ASSERT(TimerOrWaitFired == true);
    PARAOS_ATTR_UNUSED_VAR(TimerOrWaitFired);
    auto* this_ptr = reinterpret_cast<timer*>(lpParameter);
    this_ptr->run();
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

using Timer PARAOS_DEPRECATED("use paraos::timer") = timer;

}  // namespace paraos

#endif /* PARAOS_TIMER_HPP */
