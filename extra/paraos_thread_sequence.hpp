/// @file paraos_thread_sequence_v2,hpp
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

#ifndef PARAOS_THREAD_SEQUENCE_HPP
#define PARAOS_THREAD_SEQUENCE_HPP

#include "etl/callback_timer.h"
#include "gsl/gsl"
#include "paraos_bool_atomic.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_thread.hpp"

namespace paraos {

class IThreadSequence : public Thread {
  typedef etl::delegate<void(void)> callback_type;

 protected:
  IThreadSequence(
      const std::string name, const std::size_t stack_depth,
      const ThreadPriority priority, uint32_t period_in_us,
      etl::icallback_timer& timer_controller)
      : Thread{name, stack_depth, priority},
        period_in_us_{period_in_us},
        timer_controller_{timer_controller} {}

  virtual ~IThreadSequence() {}

  /// @brief Run is called in loop wrapper in separate RTOS thread until
  /// anything call Break().
  void Run() override {
    // Wait semaphore before try run all methods in array. It's allows call
    // methods with a user-defined period (period with witch user code calls
    // the method NotifyGive()).
    new_cycle_ready_sem_.Take(max_delay);

    if (timer_controller_.tick(nticks_)) {
      nticks_ = period_in_us_;
    } else {
      nticks_ += period_in_us_;
    }
  }

  /// @brief Force break thread execute. Useful in unit tests.
  void Break() {
    const paraos::CriticalSection critical;

    // Run() no more called.
    Thread::SetNeedWhile(false);

    // Give notify for last call all registered methods task_sequence_. It's
    // necessary for resume Run() from blocking mode and complete one iteration.
    // After Run() complete, thread wrapper can safely delete thread (because
    // above we call Thread::SetNeedWhile(false)) and the thead object can be
    // safely deleted in thead dtor.
    NotifyGive();
  }

 public:
  /// @brief Register delegate for periodic execute.
  /// @param[in] callback: Delegete that needs to be registered.
  /// @param[in] freq: If set 0.0, callback will be called on each user called
  /// NotifyGive().
  /// @param[in] repeating: true if need periodic call, false if need call at
  /// once.
  /// @return return etl::timer::id::NO_TIMER if delegate not registered. In
  /// other case return valid timer id in range [0 .. 254].
  auto Registered(callback_type& callback, float freq, bool repeating)
      -> etl::timer::id::type {
    paraos::CriticalSection critical;
    auto timer_id = timer_controller_.register_timer(
        callback, FreqToPeriod(freq), repeating);

    if (timer_id != etl::timer::id::NO_TIMER) {
      timer_controller_.start(timer_id);
    }

    return timer_id;
  }

  auto Unregistered(etl::timer::id::type timer_id) {
    return timer_controller_.unregister_timer(timer_id);
  }

  auto SetFreq(etl::timer::id::type timer_id, float freq_) {
    bool is_period_updated{false};

    paraos::CriticalSection critical;

    if (timer_controller_.set_period(timer_id, FreqToPeriod(freq_))) {
      // Is timer period successfully update, that's mean timer was stopped,
      // need start it again.
      is_period_updated = timer_controller_.start(timer_id);
    }

    return is_period_updated;
  }

  /// @brief Give notify for start new cycle of scheduling tasks, written in
  /// task_sequence_.
  /// @note User code must call this method at regular intervals, for example -
  /// in a timer overflow interrupt.
  /// @return
  bool NotifyGive(bool is_isr = false) {
    return new_cycle_ready_sem_.Give(is_isr);
  }

  /// Methods definitions ------------------------------------------------------
 private:
  [[nodiscard]] uint32_t FreqToPeriod(float freq) {
    // in Ctor ThreadSequence, user set period for called NotifyGive() by user
    // code. In this case, we calculate period in microseconds from frequency.
    uint32_t period_us{period_in_us_};
    if (freq != 0.0) {
      constexpr float us_in_sec{1000000};
      period_us = gsl::narrow_cast<uint32_t>(1.0f / freq * us_in_sec);
    }

    return period_us;
  }

  /// Variable definitions -----------------------------------------------------
 private:
  SemaphoreBinary new_cycle_ready_sem_;

  // Period in microseconds between user code calling NotifyGive(). User code
  // must provide this information correctly.
  const uint32_t period_in_us_;

  /// @brief Scheduler, based on callback timers.
  etl::icallback_timer& timer_controller_;

  // if set nticks_ to zero, delegate will be called after delay
  // period_in_us_. It's not useful for tests.
  uint32_t nticks_{period_in_us_};
};

template <uint_least8_t MAX_TASKS = 4>
class ThreadSequence : public IThreadSequence {
  typedef etl::delegate<void(void)> callback_type;

 public:
  ThreadSequence(
      const std::string name, const std::size_t stack_depth,
      const ThreadPriority priority, uint32_t period_in_us)
      : IThreadSequence{
            name, stack_depth, priority, period_in_us, timer_controller_} {
    // Run() method must call in forever loop periodical.
    Thread::SetNeedWhile(true);

    // Method below create thread and scheduling it's for execute in RTOS (or
    // windows/unix).
    Thread::Start();

    // Allow execute all timers, registered in timer_controller_
    timer_controller_.enable(true);
  }

  virtual ~ThreadSequence() { Break(); }

  /// @brief Force break thread execute. Useful in unit tests.
  void Break() { IThreadSequence::Break(); }

 private:
  etl::callback_timer<MAX_TASKS> timer_controller_;
  SemaphoreBinary new_cycle_ready_sem_;
};
}  // namespace paraos

#endif /* PARAOS_THREAD_SEQUENCE_HPP */
