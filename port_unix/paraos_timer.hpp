/// @file paraos_timer.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_TIMER_HPP
#define PARAOS_TIMER_HPP

#include <atomic>
#include <chrono>
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

/// @brief Class that provides software timers. To create a timer, user code
/// must provide a custom class derived from paraos::timer.
///
class timer {
 public:
  /// @brief Software timer constructor.
  /// @param[in] period_ms: Period in milliseconds between calls of the
  /// user-overridden run() method.
  /// @param[in] start_immediately: If true, the user code does not need to call
  /// start() to run the timer. Otherwise, start() must be called to run the
  /// timer.
  /// @param[in] is_auto_reload: If true, the user-overridden run() method is
  /// called periodically with the given period. If false, run() is called once
  /// after the period delay. If is_auto_reload == false and run() must be
  /// called again, call start().
  /// @param[in] name: Human-readable string. Useful for debugging.
  explicit timer(
      std::size_t period_ms, bool start_immediately = false,
      bool is_auto_reload = true, std::string_view name = "Timer")
      : period_ms_{period_ms}, is_auto_reload_{is_auto_reload}, name_{name} {
    if (start_immediately) {
      start();
    }
  }

  virtual ~timer() { stop(); }

  /// @brief Starts the timer. If is_auto_reload is false, run() is called once
  /// after <period_ms> delay. Otherwise, run() is called periodically with
  /// <period_ms> interval. Use change_period() to change the period.
  /// @param[in] max_block_time: Backward compatibility for the FreeRTOS API.
  /// Not used on Unix.
  /// @param[in] is_isr: Backward compatibility for the FreeRTOS API. Not used
  /// on Unix.
  /// @return Returns true if the timer was successfully started, false
  /// otherwise.
  auto start(paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> isr_bool {
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    const std::scoped_lock lock{mutex_};

    is_stop_requested_ = false;
    next_deadline_ =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(period_ms_);

    if (is_running_) {
      period_changed_ = true;
      wake_worker();
      return isr_bool{true};
    }

    is_running_ = true;
    period_changed_ = false;
    worker_.emplace([this](const paraos::stop_token& token) -> void {
      timer_loop(token);
    });

    return isr_bool{true};
  }

  /// @brief Changes the interval between periodic run() calls when the timer
  /// is in periodic mode (is_auto_reload == true), or changes the delay before
  /// run() is called after start() in one-shot mode (is_auto_reload == false).
  /// @param[in] period_ms: New period value in milliseconds.
  /// @param[in] max_block_time: Backward compatibility for the FreeRTOS API.
  /// Not used on Unix.
  /// @param[in] is_isr: Backward compatibility for the FreeRTOS API. Not used
  /// on Unix.
  /// @return Returns true if the period was successfully updated, false
  /// otherwise.
  auto change_period(
      std::size_t period_ms, paraos::delay_type max_block_time = max_delay,
      bool is_isr = false) -> isr_bool {
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    const std::scoped_lock lock{mutex_};

    period_ms_ = period_ms;
    if (is_running_) {
      next_deadline_ = std::chrono::steady_clock::now() +
                       std::chrono::milliseconds(period_ms_);
      period_changed_ = true;
      wake_worker();
    }

    return isr_bool{true};
  }

  /// @brief Stops the software timer. After stop() is called, run() is not
  /// called until start() is called again.
  /// @param[in] max_block_time: Backward compatibility for the FreeRTOS API.
  /// Not used on Unix.
  /// @param[in] is_isr: Backward compatibility for the FreeRTOS API. Not used
  /// on Unix.
  /// @return Returns true if the timer was successfully stopped, false
  /// otherwise.
  auto stop(paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> isr_bool {
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    {
      const std::scoped_lock lock{mutex_};
      is_stop_requested_ = true;
      is_running_ = false;
      wake_worker();
    }

    if (!is_in_run_.load(std::memory_order_acquire)) {
      worker_ = std::nullopt;
    }

    return isr_bool{true};
  }

  /// @brief Resets the software timer. After reset(), the delay before the
  /// next run() call is recalculated relative to the current time. If the timer
  /// was stopped, run() is scheduled according to <period_ms> and
  /// <is_auto_reload>.
  /// @param[in] max_block_time: Backward compatibility for the FreeRTOS API.
  /// Not used on Unix.
  /// @param[in] is_isr: Backward compatibility for the FreeRTOS API. Not used
  /// on Unix.
  /// @return Returns true if the timer was successfully reset, false
  /// otherwise.
  auto reset(paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> isr_bool {
    return start(max_block_time, is_isr);
  }

  /// @brief Method called periodically in the software timer context according
  /// to the timer creation parameters.
  virtual void run() {
    // User code must override this method in derivate class.
    PARAOS_CHECK_ASSERT(false);
  }

  /// @brief Rule of five.
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
  void timer_loop(const paraos::stop_token& token) {
    while (!token.stop_requested() && !is_stop_requested()) {
      std::size_t period_ms{};
      bool is_auto_reload{false};
      std::chrono::steady_clock::time_point deadline{};

      {
        const std::scoped_lock lock{mutex_};
        period_ms = period_ms_;
        is_auto_reload = is_auto_reload_;
        deadline = next_deadline_;
      }

      const auto now = std::chrono::steady_clock::now();
      if (deadline > now) {
        const auto remaining = deadline - now;
        (void)wake_sem_.try_acquire_for(remaining);
      }

      {
        const std::scoped_lock lock{mutex_};
        if (is_stop_requested_) {
          break;
        }
        if (std::chrono::steady_clock::now() < next_deadline_) {
          continue;
        }
      }

      try {
        is_in_run_.store(true, std::memory_order_release);
        run();
        is_in_run_.store(false, std::memory_order_release);
      } catch (...) {
        is_in_run_.store(false, std::memory_order_release);
        const std::scoped_lock lock{mutex_};
        is_running_ = false;
        is_stop_requested_ = true;
        break;
      }

      const std::scoped_lock lock{mutex_};
      if (is_stop_requested_) {
        break;
      }

      if (!is_auto_reload) {
        is_running_ = false;
        break;
      }

      const auto after_run = std::chrono::steady_clock::now();
      if (period_changed_) {
        period_changed_ = false;
        next_deadline_ = after_run + std::chrono::milliseconds(period_ms);
      } else {
        next_deadline_ += std::chrono::milliseconds(period_ms);
      }
      const auto max_deadline = after_run + std::chrono::milliseconds(period_ms);
      if (next_deadline_ < after_run) {
        next_deadline_ = after_run;
      } else if (next_deadline_ > max_deadline) {
        next_deadline_ = max_deadline;
      }
    }
  }

  [[nodiscard]] auto is_stop_requested() -> bool {
    const std::scoped_lock lock{mutex_};
    return is_stop_requested_;
  }

  void wake_worker() {
    while (wake_sem_.try_acquire()) {
    }
    wake_sem_.release();
  }

 private:
  /// @brief Interval between run() calls when is_auto_reload_ == true.
  /// Otherwise, it is the delay before run() is called after start(). If
  /// start_immediately == true was passed to the constructor, period_ms_ is the
  /// delay before run() is called after the software timer object is
  /// constructed.
  std::size_t period_ms_;

  /// @brief If true, run() is called periodically with period_ms_. Otherwise,
  /// run() is called once after the delay provided by period_ms_ following
  /// start() (or after the software timer object is constructed if
  /// <start_immediately == true>).
  bool is_auto_reload_;

  /// @brief Human-readable timer name.
  std::string_view name_;

  /// @brief Protects timer state shared between the worker thread and user
  /// calls.
  paraos::mutex mutex_;

  /// @brief Used to wake the worker thread on stop, start, or period change.
  paraos::binary_semaphore wake_sem_{0};

  /// @brief Lazy-created worker thread that drives the timer loop.
  std::optional<paraos::jthread> worker_;

  /// @brief True while the timer is scheduled to run.
  bool is_running_{false};

  /// @brief Set to true to request the worker thread to exit.
  bool is_stop_requested_{false};

  /// @brief Set to true when the period is changed while the timer is running.
  bool period_changed_{false};

  /// @brief True while run() is executing. Guards against joining the worker
  /// thread from within run().
  std::atomic<bool> is_in_run_{false};

  /// @brief Absolute deadline for the next run() call.
  std::chrono::steady_clock::time_point next_deadline_;
};

using Timer PARAOS_DEPRECATED("use paraos::timer") = timer;

}  // namespace paraos

#endif /* PARAOS_TIMER_HPP */
