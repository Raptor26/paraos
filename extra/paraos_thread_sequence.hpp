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
#include "etl/delegate.h"
#include "gsl/gsl"
#include "paraos_bool_atomic.hpp"
#include "paraos_isr.hpp"
#include "paraos_mutex.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_thread.hpp"

namespace paraos {

#if PARAOS_THREAD_SEQUENCE_USING_VIRTUAL
#define PARAOS_THREAD_SEQUENCE_VIRTUAL virtual
#else
#define PARAOS_THREAD_SEQUENCE_VIRTUAL
#endif

/// @brief Provided thread for execute delegates.
///
/// @warning No delegates should use blocking paraos API. For example,
/// - sem.Take(100) - bad idea, because all delegates in the thread will blocked
///                   for 100 ms.
/// - sem.Take(0) - good. If no semaphore for take, method return control to
///                 delegate immediately. Remember, all blocking api return
///                 status, indicates is API calls successfully.
class IThreadSequence : public Thread {
  using callback_type = etl::delegate<void(void)>;
  using try_lock_type = etl::delegate<bool(void)>;
  using lock_type = etl::delegate<void(void)>;
  using unlock_type = etl::delegate<void(void)>;

 protected:
  // String copy here is needed because of the delayed thread initialization -
  // address of it's name could be invalid later.
  // NOLINTBEGIN(performance-unnecessary-value-param)
  IThreadSequence(
      const std::string name, const std::size_t stack_depth,
      const ThreadPriority priority, uint32_t period_in_us,
      etl::icallback_timer &timer_controller)
      : Thread{name, stack_depth, priority},
        period_in_us_{period_in_us},
        timer_controller_{timer_controller} {}

  /// @brief Run is called in loop wrapper in separate RTOS thread until
  /// anything call Break().
  PARAOS_THREAD_SEQUENCE_VIRTUAL void Run() override {
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
  // NOLINTEND(performance-unnecessary-value-param)

 public:
  /// @brief Register delegate for periodic execute.
  ///
  /// @warning All registered delegates execute in one thread. That's mean, no
  /// delegate should use blocking API.
  /// For example, if delegate call 'sem.Take(delay_ms)' and 'delay_ms > 0', all
  /// delegates in the thread will blocked for specfied period (most likely,
  /// this is not behavior you need). Timeout in all paraos API must be set as
  /// zero!
  /// - sem.Take(100) - bad;
  /// - sem.Take(0) - good;
  ///
  /// @param[in] callback: Delegate that needs to be registered.
  /// @param[in] freq: If set 0.0, callback will be called on each user called
  /// NotifyGive().
  /// @param[in] repeating: true if need periodic call, false if need call at
  /// once.
  ///
  /// @return return etl::timer::id::NO_TIMER if delegate not registered. In
  /// other case return valid timer id in range [0 .. 254].
  PARAOS_THREAD_SEQUENCE_VIRTUAL auto Register(
      callback_type &callback, float freq, bool repeating)
      -> etl::timer::id::type {
    const paraos::CriticalSection critical;
    auto timer_id = timer_controller_.register_timer(
        callback, FreqToPeriod(freq), repeating);

    if (timer_id != etl::timer::id::NO_TIMER) {
      timer_controller_.start(timer_id);
      ++registered_delegates_numb;
    }

    return timer_id;
  }

  /// @brief Delete delegate from periodic execute.
  ///
  /// @param[in] timer_id: Delegate id, which needs for delete from queue
  /// executor.
  ///
  /// @return true if delegate successfully deleted, false in otherwise.
  PARAOS_THREAD_SEQUENCE_VIRTUAL auto Unregister(etl::timer::id::type timer_id)
      -> bool {
    const paraos::CriticalSection critical;
    auto is_unregistered = timer_controller_.unregister_timer(timer_id);

    if (is_unregistered) {
      --registered_delegates_numb;
    }
    return is_unregistered;
  }

  /// @brief Change freq for delegate execution.
  ///
  /// @param[in] timer_id: Delegate id whose execution frequency will be
  /// changed.
  /// @param[in] freq_: new frequency for periodic call delegate.
  ///
  /// @return true if frequency changed successfully, false in otherwise.
  PARAOS_THREAD_SEQUENCE_VIRTUAL auto SetFreq(
      etl::timer::id::type timer_id, float freq_) -> bool {
    bool is_period_updated{false};

    const paraos::CriticalSection critical;

    if (timer_controller_.set_period(timer_id, FreqToPeriod(freq_))) {
      // Is timer period successfully update, that's mean timer was stopped,
      // need start it again.
      is_period_updated = timer_controller_.start(timer_id);
    }

    return is_period_updated;
  }

  [[nodiscard]] auto GiveRegisteredDelegatesNumb() const -> size_t {
    const paraos::CriticalSection critical;
    return registered_delegates_numb;
  }

  /// @brief Give notify for start new cycle of scheduling tasks, written in
  /// timer_controller_.
  /// @note User code must call this method at regular intervals, for example -
  /// in a timer overflow interrupt.
  /// @return
  PARAOS_THREAD_SEQUENCE_VIRTUAL auto NotifyGive(bool is_isr = false)
      -> ISRbool {
    return new_cycle_ready_sem_.Give(is_isr);
  }

  /// @brief Return frequency which thread execute. Relative to this frequency,
  /// the periods for calling delegates are calculated.
  ///
  /// @return Main frequency in Hz.
  [[nodiscard]] PARAOS_THREAD_SEQUENCE_VIRTUAL auto GetMainFreq() const
      -> float {
    // Convert microseconds to sec.
    const float main_freq = (static_cast<float>(period_in_us_)) * 0.000001;

    return static_cast<float>(1.0) / main_freq;
  }

  /// @brief Force break thread execute. Useful in unit tests.
  PARAOS_THREAD_SEQUENCE_VIRTUAL void Break() {
    const paraos::CriticalSection critical;

    // Run() no more called.
    Thread::SetNeedWhile(false);

    // Give notify for last call all registered methods timer_controller_. It's
    // necessary for resume Run() from blocking mode and complete one iteration.
    // After Run() complete, thread wrapper can safely delete thread (because
    // above we call Thread::SetNeedWhile(false)) and the thead object can be
    // safely deleted in thead dtor.
    NotifyGive();
  }

  ~IThreadSequence() override = default;

  /// @brief Five rule.
  IThreadSequence(IThreadSequence &&other) = delete;
  auto operator=(IThreadSequence &&other) -> IThreadSequence & = delete;
  auto operator=(const IThreadSequence &other) -> IThreadSequence & = delete;
  IThreadSequence(const IThreadSequence &other) = delete;

  /// Methods definitions ------------------------------------------------------
 private:
  [[nodiscard]] auto FreqToPeriod(float freq) const -> uint32_t {
    // in Ctor ThreadSequence, user set period for called NotifyGive() by user
    // code. In this case, we calculate period in microseconds from frequency.
    uint32_t period_us{period_in_us_};
    if (freq != 0.0) {
      constexpr float us_in_sec{1000000};
      period_us = gsl::narrow_cast<uint32_t>(1.0F / freq * us_in_sec);
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
  etl::icallback_timer &timer_controller_;

  // if set nticks_ to zero, delegate will be called after delay
  // period_in_us_. It's not useful for tests.
  uint32_t nticks_{period_in_us_};

  /// @brief Indicates how many delegates registered in thread sequence.
  std::size_t registered_delegates_numb{0};
};

template <uint_least8_t MAX_TASKS = 4>
class ThreadSequence : public IThreadSequence {
 public:
  // String copy here is needed because of the delayed thread initialization -
  // address of it's name could be invalid later.
  // NOLINTBEGIN(performance-unnecessary-value-param)

  /// @brief Create separate thread for execute registered delegates.
  ///
  /// @tparam MAX_TASKS - Max registered delegates in one time.
  ///
  /// @param[in] name: Thread name, whose  context is provided for execute
  /// registered delegates.
  /// @param[in] stack_depth: Stack depth in bytes for thread.
  /// @param[in] priority: Thread priority.
  /// @param[in] period_in_us: Period in microseconds, between user code call
  /// NotifyGive(). User code responsible for specifying this parameter, which
  /// corresponding to the actual call period NotifyGive().
  /// @param[in] is_need_start: If set true, thread will creat in Ctor, if set
  /// false, thread will not created. Sef false may be useful in unit tests.
  ThreadSequence(
      const std::string name, const std::size_t stack_depth,
      const ThreadPriority priority, uint32_t period_in_us,
      bool is_need_start = true)
      : IThreadSequence{
            name, stack_depth, priority, period_in_us, timer_controller_} {
    // Run() method must call in forever loop periodical.
    Thread::SetNeedWhile(true);

    // In unit test is_need_start == false,
    if (is_need_start) {
      // Method below create thread and scheduling it's for execute in RTOS (or
      // windows/unix).
      Thread::Start();
    }

    // Allow execute all timers, registered in timer_controller_.
    timer_controller_.enable(true);
  }
  // NOLINTEND(performance-unnecessary-value-param)

  ~ThreadSequence() override { IThreadSequence::Break(); }

  /// @brief Five rule.
  ThreadSequence(ThreadSequence &&other) = delete;
  auto operator=(ThreadSequence &&other) -> ThreadSequence & = delete;
  auto operator=(const ThreadSequence &other) -> ThreadSequence & = delete;
  ThreadSequence(const ThreadSequence &other) = delete;

  /// Variable definitions -----------------------------------------------------
 private:
  etl::callback_timer<MAX_TASKS> timer_controller_;
};
}  // namespace paraos

#endif /* PARAOS_THREAD_SEQUENCE_HPP */
