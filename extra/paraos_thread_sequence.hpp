/// @file paraos_thread_sequence_v2.hpp
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

#include <cstdlib>

#include "etl/callback_timer.h"
#include "etl/delegate.h"
#include "gsl/gsl"
#include "paraos_bool_atomic.hpp"
#include "paraos_config.hpp"
#include "paraos_isr.hpp"
#include "paraos_mutex.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_thread.hpp"

namespace paraos {

/// @brief Attributes for configuring a thread sequence.
///
/// This structure extends the base thread attributes and includes an additional
/// field for specifying the period between notifications.
struct IThreadSequenceAttr : public paraos::ThreadAttr {
  /// @brief The period in microseconds between `NotifyGive()` calls.
  /// The user code is responsible for calling `NotifyGive()` at this interval.
  uint32_t period_in_us{0};
};

/// @brief Provides a thread for executing delegates.
///
/// @warning Delegates should not use blocking ParaOS API calls. For example:
///     - `sem.Take(100)` – Bad, as it blocks all delegates in the thread for
///        100 ms.
///     - `sem.Take(0)` – Good, as it returns immediately if the semaphore is
///        unavailable.
///
/// All blocking API calls should return a status indicating success or failure.
class IThreadSequence : public paraos::Base {
 public:
  using callback_type = etl::delegate<void()>;

 protected:
  // String copy is needed due to delayed thread initialization; the address of
  // its name could be invalid later.
  // NOLINTBEGIN(performance-unnecessary-value-param)
  IThreadSequence(
      const IThreadSequenceAttr &attr, etl::icallback_timer &timer_controller,
      bool thread_start_flag = true)
      : thread_{attr, thread_start_flag},
        period_in_us_{attr.period_in_us},
        timer_controller_{timer_controller} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<
            IThreadSequence, &IThreadSequence::Run>(*this));
  }
  // NOLINTEND(performance-unnecessary-value-param)

 public:
  ~IThreadSequence() override = default;

  /// @brief Deleted move constructor and assignment operators to enforce
  /// non-copyable and non-movable semantics.
  IThreadSequence(IThreadSequence &&other) = delete;
  auto operator=(IThreadSequence &&other) -> IThreadSequence & = delete;
  auto operator=(const IThreadSequence &other) -> IThreadSequence & = delete;
  IThreadSequence(const IThreadSequence &other) = delete;

  /// @brief Converts frequency in Hz to period in microseconds.
  ///
  /// @param[in] freq: Frequency in Hz.
  ///
  /// @return Period in microseconds.
  [[nodiscard]] auto FreqToPeriod(float freq) const {
    // Calculate period in microseconds from frequency.
    uint32_t period_us{period_in_us_};
    if (freq != 0.0F) {
      constexpr float us_in_sec{1000000.0F};
      period_us =
          gsl::narrow_cast<decltype(period_us)>(1.0F / freq * us_in_sec);
    }
    return period_us;
  }

  /// @brief Registers a delegate for periodic execution.
  ///
  /// @warning All registered delegates execute in a single thread. Therefore,
  /// no delegate should use blocking API calls.
  ///
  /// Example of problematic usage:
  /// - `sem.Take(100)` – Bad, as it blocks all delegates for 100 ms.
  /// - `sem.Take(0)` – Good, as it returns immediately if the semaphore is
  /// unavailable.
  ///
  /// @param[in] callback: Delegate to be registered.
  /// @param[in] freq: Execution frequency in Hz. If set to `0.0`, the callback
  /// will be called each time `NotifyGive()` is called.
  /// @param[in] repeating: Set to `true` for periodic execution, or `false` for
  /// a one-time call.
  ///
  /// @return Returns `etl::timer::id::NO_TIMER` if registration fails.
  ///         Otherwise, returns a valid timer ID in the range `[0 .. 254]`.
  PARAOS_POLYMORPHIC_EXTRA auto Register(
      callback_type &callback, float freq, bool repeating)
      -> etl::timer::id::type {
    const paraos::CriticalSection critical;
    auto timer_id = timer_controller_.register_timer(
        callback, FreqToPeriod(freq), repeating);

    if (timer_id != etl::timer::id::NO_TIMER) {
      timer_controller_.start(timer_id);
      ++registered_delegates_numb_;
    }

    return timer_id;
  }

  /// @brief Removes a delegate from periodic execution.
  ///
  /// @param[in] timer_id: ID of the delegate to be removed. If delegate
  /// successfully unregistered, timer_id will be set to
  /// 'etl::timer::id::NO_TIMER'.
  ///
  /// @return Returns `true` if the delegate was successfully removed, `false`
  /// otherwise.
  PARAOS_POLYMORPHIC_EXTRA auto Unregister(etl::timer::id::type &timer_id)
      -> bool {
    const paraos::CriticalSection critical;
    auto is_unregistered = timer_controller_.unregister_timer(timer_id);
    if (is_unregistered) {
      --registered_delegates_numb_;
      timer_id = etl::timer::id::NO_TIMER;
    }

    return is_unregistered;
  }

  /// @brief Changes the execution frequency of a registered delegate.
  ///
  /// @param[in] timer_id: ID of the delegate whose frequency will be changed.
  /// @param[in] freq: New frequency for periodic delegate execution in Hz.
  ///
  /// @return Returns `true` if the frequency was successfully updated, `false`
  /// otherwise.
  PARAOS_POLYMORPHIC_EXTRA auto SetFreq(
      etl::timer::id::type timer_id, float freq) -> bool {
    bool is_period_updated{false};
    const paraos::CriticalSection critical;
    if (timer_controller_.set_period(timer_id, FreqToPeriod(freq))) {
      // If the timer period is successfully updated, restart the timer.
      is_period_updated = timer_controller_.start(timer_id);
    }
    return is_period_updated;
  }

  /// @brief Returns the number of registered delegates.
  [[nodiscard]] auto GiveRegisteredDelegatesNumb() const {
    const paraos::CriticalSection critical;
    return registered_delegates_numb_;
  }

  /// @brief Initiates a new scheduling cycle by notifying the thread sequence.
  ///
  /// This method is designed to be invoked at regular intervals, typically from
  /// a timer overflow interrupt or similar periodic event.
  ///
  /// @warning Avoid using default arguments in virtual methods. For example,
  /// do not use `auto NotifyGive(bool is_isr = false)` because `NotifyGive()`
  /// may be declared as virtual if `PARAOS_USING_POLYMORPHIC_EXTRA` is defined.
  /// Always explicitly specify the `is_isr` parameter when calling this method.
  ///
  /// @param[in] is_isr Indicates whether this method is being called from an
  /// interrupt service routine (ISR). Set to `true` if called from an ISR,
  /// otherwise set to `false`.
  ///
  /// @return Returns `true` if the semaphore was successfully signaled, or
  /// `false` if the operation failed (e.g., due to resource constraints or
  /// invalid state).
  PARAOS_POLYMORPHIC_EXTRA auto NotifyGive(bool is_isr) -> paraos::ISRbool {
    return new_cycle_ready_sem_.Give(is_isr);
  }

  /// @brief Returns the main frequency of the thread sequence in Hz.
  /// The periods for calling delegates are calculated based on this frequency.
  ///
  /// @return The main frequency in Hz.
  [[nodiscard]] PARAOS_POLYMORPHIC_EXTRA auto GetMainFreq() const -> float {
    // Convert microseconds to seconds.
    const float main_freq = static_cast<float>(period_in_us_) * 0.000001F;
    return 1.0F / main_freq;
  }

  /// @brief Completes thread execution.
  ///
  /// @param[in] is_dynamic: Set to `true` if the ThreadSequence was created
  /// on the heap and is not managed by user code or smart pointers.
  /// In this case, `CooperativeScheduling()` will be removed from the heap
  /// after the thread completes all work. Otherwise, set to `false`.
  PARAOS_POLYMORPHIC_EXTRA void Finish(bool is_dynamic) {
    const paraos::CriticalSection critical;
    paraos::Base *deferred_destroy{nullptr};

    if (is_dynamic) {
      deferred_destroy = this;
    }

    thread_.Finished(deferred_destroy);

    // Notify the thread to complete the final iteration of all registered
    // methods. This ensures that `Run()` exits blocking mode and completes one
    // more cycle.
    //
    // After `Run()` completes, the thread wrapper can safely delete the thread
    // (since `Thread::SetNeedWhile(false)` was called earlier),
    // and the thread object can be safely deleted in the thread destructor.
    NotifyGive(false);
  }

 private:
  /// @brief Main loop function executed by the thread.
  /// Runs until `Break()` is called.
  PARAOS_POLYMORPHIC_EXTRA void Run() {
    // Wait for the semaphore before processing all registered delegates.
    // This allows delegates to be called with a user-defined period.
    new_cycle_ready_sem_.Take(paraos::max_delay);
    if (timer_controller_.tick(nticks_)) {
      nticks_ = period_in_us_;
    } else {
      nticks_ += period_in_us_;
    }
  }

 private:
  /// Binary semaphore for synchronization.
  SemaphoreBinary new_cycle_ready_sem_;

  /// Thread instance.
  paraos::Thread thread_;

  /// Period in microseconds between `NotifyGive()` calls.
  const uint32_t period_in_us_;

  /// Callback timer controller.
  etl::icallback_timer &timer_controller_;

  /// Number of ticks since last notification.
  uint32_t nticks_{period_in_us_};

  /// Number of registered delegates.
  std::size_t registered_delegates_numb_{0};
};

struct ThreadSequenceAttr : public paraos::IThreadSequenceAttr {};

/// @brief Concrete implementation of `IThreadSequence`.
template <uint_least8_t MAX_TASKS = 4>
class ThreadSequence : public IThreadSequence {
 public:
  /// @brief Creates a separate thread for executing registered delegates.
  ///
  /// @param[in] attr: Thread sequence attributes.
  /// @param[in] thread_start_flag: Flag indicating whether to start the thread
  /// immediately. Useful in test environments without multithreading.
  explicit ThreadSequence(
      const ThreadSequenceAttr &attr, bool thread_start_flag = true)
      : IThreadSequence{attr, timer_controller_, thread_start_flag} {
    // Enable all timers registered in the timer controller.
    timer_controller_.enable(true);
  }

  ~ThreadSequence() override = default;

  /// @brief Deleted move constructor and assignment operators to enforce
  /// non-copyable and non-movable semantics.
  ThreadSequence(ThreadSequence &&other) = delete;
  auto operator=(ThreadSequence &&other) -> ThreadSequence & = delete;
  auto operator=(const ThreadSequence &other) -> ThreadSequence & = delete;
  ThreadSequence(const ThreadSequence &other) = delete;

 private:
  /// Callback timer with a fixed number of tasks.
  etl::callback_timer<MAX_TASKS> timer_controller_;
};

}  // namespace paraos

#endif /* PARAOS_THREAD_SEQUENCE_HPP */