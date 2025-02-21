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

struct IThreadSequenceAttr : public paraos::ThreadAttr {
  /// @brief The period in microseconds between NotifyGive() calls, which the
  /// user code is obligated to perform.
  uint32_t period_in_us{0};
};

/// @brief Provides a thread for executing delegates.
///
/// @warning No delegate should use blocking ParaOS API calls. For example:
/// - `sem.Take(100)` – Bad idea, as it will block all delegates in the thread
///                     for 100 ms.
/// - `sem.Take(0)` – Good. If the semaphore is unavailable, the method
///                   immediately returns control to the delegate.
///
/// Remember, all blocking API calls return a status indicating
/// whether the call was successful.
class IThreadSequence : public paraos::Base {
  using callback_type = etl::delegate<void()>;

 protected:
  // String copy here is needed because of the delayed thread initialization -
  // address of it's name could be invalid later.
  // NOLINTBEGIN(performance-unnecessary-value-param)
  IThreadSequence(
      const IThreadSequenceAttr &attr, etl::icallback_timer &timer_controller,
      bool thread_start_flag = true)
      : thread_{attr, thread_start_flag},
        period_in_us_{attr.period_in_us},
        timer_controller_{timer_controller} {
    thread_.RegisterDelegate(paraos::thread_delegate_type::create<
                             IThreadSequence, &IThreadSequence::Run>(*this));
  }
  // NOLINTEND(performance-unnecessary-value-param)

 public:
  ~IThreadSequence() override = default;

  /// @brief Five rule. --------------------------------------------------------
  IThreadSequence(IThreadSequence &&other) = delete;
  auto operator=(IThreadSequence &&other) -> IThreadSequence & = delete;
  auto operator=(const IThreadSequence &other) -> IThreadSequence & = delete;
  IThreadSequence(const IThreadSequence &other) = delete;

  /// --------------------------------------------------------------------------

  /// @brief Registers a delegate for periodic execution.
  ///
  /// @warning All registered delegates execute in a single thread.
  /// This means that no delegate should use blocking API calls.
  ///
  /// For example, if a delegate calls `sem.Take(delay_ms)` with `delay_ms > 0`,
  /// all delegates in the thread will be blocked for the specified period
  /// (which is likely not the desired behavior).
  /// Timeout values in all ParaOS API calls must be set to zero!
  /// - `sem.Take(100)` – Bad;
  /// - `sem.Take(0)` – Good.
  ///
  /// @param[in] callback Delegate to be registered.
  /// @param[in] freq If set to `0.0`, the callback will be called
  ///                 each time the user calls `NotifyGive()`.
  /// @param[in] repeating Set to `true` for periodic execution,
  ///                      or `false` for a one-time call.
  ///
  /// @return Returns `etl::timer::id::NO_TIMER` if the delegate was not
  /// registered. Otherwise, returns a valid timer ID in the range `[0 .. 254]`.
  PARAOS_THREAD_SEQUENCE_VIRTUAL auto Register(
      callback_type &callback, float freq,
      bool repeating) -> etl::timer::id::type {
    const paraos::CriticalSection critical;
    auto timer_id = timer_controller_.register_timer(
        callback, FreqToPeriod(freq), repeating);

    if (timer_id != etl::timer::id::NO_TIMER) {
      timer_controller_.start(timer_id);
      ++registered_delegates_numb;
    }

    return timer_id;
  }

  /// --------------------------------------------------------------------------

  /// @brief Removes a delegate from periodic execution.
  ///
  /// @param[in] timer_id Delegate ID to be removed from the execution queue.
  ///
  /// @return Returns true if the delegate was successfully deleted,
  /// false otherwise.
  PARAOS_THREAD_SEQUENCE_VIRTUAL auto Unregister(etl::timer::id::type timer_id)
      -> bool {
    const paraos::CriticalSection critical;
    auto is_unregistered = timer_controller_.unregister_timer(timer_id);

    if (is_unregistered) {
      --registered_delegates_numb;
    }
    return is_unregistered;
  }

  /// --------------------------------------------------------------------------

  /// @brief Changes the execution frequency of a registered delegate.
  ///
  /// @param[in] timer_id Delegate ID whose execution frequency will be
  /// modified.
  /// @param[in] freq_ New frequency for periodic delegate execution.
  ///
  /// @return Returns true if the frequency was successfully changed,
  /// false otherwise.
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

  /// --------------------------------------------------------------------------

  [[nodiscard]] auto GiveRegisteredDelegatesNumb() const -> size_t {
    const paraos::CriticalSection critical;
    return registered_delegates_numb;
  }

  /// --------------------------------------------------------------------------

  /// @brief Sends a notification to start a new scheduling cycle for tasks
  /// managed by `timer_controller_`.
  ///
  /// @note The user code must call this method at regular intervals, for
  /// example, in a timer overflow interrupt.
  ///
  /// @return Returns true if the semaphore was successfully given,
  /// false otherwise.
  PARAOS_THREAD_SEQUENCE_VIRTUAL auto NotifyGive(bool is_isr = false)
      -> ISRbool {
    return new_cycle_ready_sem_.Give(is_isr);
  }

  /// --------------------------------------------------------------------------

  /// @brief Returns the execution frequency of the thread.
  /// The periods for calling delegates are calculated based on this frequency.
  ///
  /// @return The main frequency in Hz.
  [[nodiscard]] PARAOS_THREAD_SEQUENCE_VIRTUAL auto GetMainFreq() const
      -> float {
    // Convert microseconds to sec.
    const float main_freq = (static_cast<float>(period_in_us_)) * 0.000001;

    return static_cast<float>(1.0) / main_freq;
  }

  /// --------------------------------------------------------------------------

  /// @brief Completes thread execution.
  ///
  /// @param[in] is_dynamic Set to true if the ThreadSequence was created
  /// on the heap and is not managed by user code or smart pointers.
  /// In this case, `CooperativeScheduling()` will be removed from the heap
  /// after the thread completes all work. Otherwise, set to false.
  PARAOS_THREAD_SEQUENCE_VIRTUAL void Finish(bool is_dynamic = false) {
    const paraos::CriticalSection critical;

    paraos::Base *deferred_destroy{nullptr};
    if (is_dynamic) {
      deferred_destroy = this;
    }

    thread_.Finished(deferred_destroy);

    // Sends a notification for the final call of all registered methods
    // in `timer_controller_`. This is necessary to resume `Run()`
    // from blocking mode and complete one iteration.
    //
    // After `Run()` completes, the thread wrapper can safely delete the thread
    // (since `Thread::SetNeedWhile(false)` was called earlier),
    // and the thread object can be safely deleted in the thread destructor.
    NotifyGive();
  }

  /// Methods definitions ------------------------------------------------------
 private:
  /// @brief Run() is called in a loop, wrapped in a separate RTOS thread,
  /// until Break() is called.
  PARAOS_THREAD_SEQUENCE_VIRTUAL void Run() {
    // Wait for the semaphore before attempting to run all methods in the array.
    // This allows methods to be called with a user-defined period
    // (the period at which the user code calls NotifyGive()).
    new_cycle_ready_sem_.Take(max_delay);

    if (timer_controller_.tick(nticks_)) {
      nticks_ = period_in_us_;
    } else {
      nticks_ += period_in_us_;
    }
  }

  /// --------------------------------------------------------------------------

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

  paraos::Thread thread_;

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

struct ThreadSequenceAttr : public paraos::IThreadSequenceAttr {};

template <uint_least8_t MAX_TASKS = 4>
class ThreadSequence : public IThreadSequence {
 public:
  /// @brief Create separate thread for execute registered delegates.
  /// @param[in] attr: Thread sequence attributes.
  /// @param[in] thread_start_flag: Flag that indicates thread start condition.
  /// May be useful in tests where there is no multithread environment needed.
  explicit ThreadSequence(
      const ThreadSequenceAttr &attr, bool thread_start_flag = true)
      : IThreadSequence{attr, timer_controller_, thread_start_flag} {
    // Allow execute all timers, registered in timer_controller_.
    timer_controller_.enable(true);
  }

  ~ThreadSequence() override = default;

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
