/// @file paraos_thread_sequence.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_THREAD_SEQUENCE_HPP
#define PARAOS_THREAD_SEQUENCE_HPP

#include <cmath>
#include <cstdlib>

#include "etl/callback_timer.h"
#include "etl/delegate.h"
#include "gsl/gsl"
#include "paraos_bool_atomic.hpp"
#include "paraos_config.hpp"
#include "paraos_exceptions.hpp"
#include "paraos_isr.hpp"
#include "paraos_mutex.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_jthread.hpp"

namespace paraos {

#define PARAOS_THREAD_SEQUENCE_UNKNOW_FILE_ID ("102")

class InvalidThreadSequenceException final : public paraos::exception {
 public:
  /// @brief Constructor for InvalidThreadSequenceException.
  ///
  /// @param file_name_ the name of the file where the exception occurred
  /// @param line_number_ the line number where the exception occurred
  InvalidThreadSequenceException(
      paraos::error_string_type file_name_,
      paraos::error_numeric_type line_number_)
      : paraos::exception(
            paraos::GetErrorText(
                "Telemetry: Incorrect thread sequence interface",
                PARAOS_THREAD_SEQUENCE_UNKNOW_FILE_ID),
            file_name_, line_number_) {}

  /// @brief Destructor for InvalidThreadSequenceException.
  ~InvalidThreadSequenceException() override = default;

  /// @brief Copy constructor for StavlinkException.
  InvalidThreadSequenceException(const InvalidThreadSequenceException &) =
      default;

  /// @brief Copy assignment operator for StavlinkException.
  auto operator=(const InvalidThreadSequenceException &)
      -> InvalidThreadSequenceException & = default;

  /// @brief Move constructor for StavlinkException.
  InvalidThreadSequenceException(InvalidThreadSequenceException &&) = default;

  /// @brief Move assignment operator for StavlinkException.
  auto operator=(InvalidThreadSequenceException &&)
      -> InvalidThreadSequenceException & = default;
};

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
  using callback_type = etl::icallback_timer::callback_type;

 protected:
  // String copy is needed due to delayed thread initialization; the address of
  // its name could be invalid later.
  // NOLINTBEGIN(performance-unnecessary-value-param)
  IThreadSequence(
      const IThreadSequenceAttr &attr, etl::icallback_timer &timer_controller,
      bool thread_start_flag = true)
      : period_in_us_{attr.period_in_us},
        timer_controller_{timer_controller},
        thread_{static_cast<const paraos::ThreadAttr &>(attr),
                [this](const paraos::stop_token &token) { Run(token); }} {
    (void)thread_start_flag;
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
  /// @param[in] is_dynamic: Kept for API compatibility; ignored. The jthread
  /// destructor handles cleanup.
  PARAOS_POLYMORPHIC_EXTRA void Finish(bool is_dynamic) {
    (void)is_dynamic;

    // Request the sequence thread to stop.
    (void)thread_.request_stop();

    // Notify the thread to complete the final iteration of all registered
    // methods. This ensures that `Run()` exits blocking mode and completes one
    // more cycle.
    NotifyGive(false);

    // Wait for the sequence thread to finish gracefully.
    thread_.join();
  }

 private:
  /// @brief Main loop function executed by the thread.
  /// Runs until a stop is requested.
  PARAOS_POLYMORPHIC_EXTRA void Run(const paraos::stop_token &token) {
    while (!token.stop_requested()) {
      // Wait for the semaphore before processing all registered delegates.
      // This allows delegates to be called with a user-defined period.
      new_cycle_ready_sem_.Take(paraos::max_delay);
      if (token.stop_requested()) {
        break;
      }
      if (timer_controller_.tick(nticks_)) {
        nticks_ = period_in_us_;
      } else {
        nticks_ += period_in_us_;
      }
    }
  }

 private:
  /// Period in microseconds between `NotifyGive()` calls.
  const uint32_t period_in_us_;

  /// Callback timer controller.
  etl::icallback_timer &timer_controller_;

  /// Binary semaphore for synchronization.
  SemaphoreBinary new_cycle_ready_sem_;

  /// Thread instance.
  paraos::jthread thread_;

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

/// @brief RAII wrapper for managing multiple delegates in a thread sequence.
///
/// This class simplifies the management of periodic or one-time callbacks
/// registered with a `paraos::IThreadSequence`. It ensures automatic
/// unregistration of all delegates upon destruction, preventing resource leaks.
///
/// Example usage:
/// ```cpp
/// RegisteredDelegates<4> delegates(thread_sequence_);
/// delegates.Register(0, callback, 1.0f, true); // Register periodic delegate
/// ```
///
/// @tparam SIZE Maximum number of delegates to manage.
template <std::size_t SIZE = 4>
class RegisteredDelegates final {
 public:
  /// @brief Construct a new RegisteredDelegates object.
  ///
  /// @param thread_sequence Reference to the thread sequence used for
  /// registering delegates.
  ///
  /// @throws paraos::InvalidThreadSequenceException if `thread_sequence` is
  /// null.
  explicit RegisteredDelegates(paraos::IThreadSequence &thread_sequence)
      : thread_sequence_{&thread_sequence} {
    ETL_ASSERT(
        thread_sequence_, ETL_ERROR(paraos::InvalidThreadSequenceException));

    // Initialize all delegate IDs as 'no timer' before any registration.
    std::fill(id_.begin(), id_.end(), etl::timer::id::NO_TIMER);
  }

  /// @brief Destroy the RegisteredDelegates object.
  ///
  /// Automatically unregisters all currently active delegates from the thread
  /// sequence.
  ~RegisteredDelegates() {
    for (std::size_t i = 0U; i < id_.size(); ++i) {
      Unregister(i);
    }
  }

  // Disable copy and assignment
  RegisteredDelegates(const RegisteredDelegates &) = delete;
  auto operator=(const RegisteredDelegates &) -> RegisteredDelegates & = delete;

  // Disable move operations (optional, can be enabled later)
  RegisteredDelegates(RegisteredDelegates &&) noexcept = delete;
  auto operator=(RegisteredDelegates &&) -> RegisteredDelegates & = delete;

  /// @brief Register a delegate at a specific position.
  ///
  /// Registers a callback with the associated thread sequence. If already
  /// registered at this position, no action is taken.
  ///
  /// @param pos Position index [0, SIZE-1) to store delegate ID.
  /// @param delegate Callback function to register.
  /// @param freq_hz Frequency (in Hz) at which the delegate should be called.
  ///                A value of zero means "run once".
  /// @param repeating Set to true for periodic execution.
  ///
  /// @return The assigned delegate ID (can be used for manual unregistration).
  ///
  /// @throws std::out_of_range if `pos >= SIZE`.
  auto Register(
      std::size_t pos, IThreadSequence::callback_type &delegate, float freq_hz,
      bool repeating) {
    auto &delegate_id = id_.at(pos);  // No need for extra cast if using size_t
    if ((freq_hz != 0.0F) && (std::isfinite(freq_hz))) {
      if ((delegate_id == etl::timer::id::NO_TIMER) && thread_sequence_) {
        delegate_id = thread_sequence_->Register(delegate, freq_hz, repeating);
      }
    }

    return delegate_id;
  }

  /// @brief Unregister a delegate at a specific position.
  ///
  /// If a delegate is registered at the specified position, it will be
  /// unregistered from the thread sequence.
  ///
  /// @param pos Position index [0, SIZE-1) to unregister.
  /// @return true if successfully unregistered, false otherwise.
  ///
  /// @throws std::out_of_range if `pos >= SIZE`.
  auto Unregister(std::size_t pos) -> bool {
    auto &delegate_id = id_.at(pos);  // Simplified without unnecessary cast
    if (thread_sequence_) {
      if (thread_sequence_->Unregister(delegate_id)) {
        delegate_id = etl::timer::id::NO_TIMER;
        return true;
      }
    }

    return false;
  }

  /// @brief Unregister, then try register delegate. Useful if need change
  /// period 'freq_hz'
  ///
  /// @param pos Position index [0, SIZE-1) to store delegate ID.
  /// @param delegate Callback function to register.
  /// @param freq_hz Frequency (in Hz) at which the delegate should be called.
  ///                A value of zero means "run once".
  /// @param repeating Set to true for periodic execution.
  ///
  /// @return The assigned delegate ID (can be used for manual unregistration).
  ///
  /// @throws std::out_of_range if `pos >= SIZE`.
  auto UnregisterThenTryRegister(
      std::size_t pos, IThreadSequence::callback_type &delegate, float freq_hz,
      bool repeating) {
    Unregister(pos);
    return Register(pos, delegate, freq_hz, repeating);
  }

 private:
  paraos::IThreadSequence *thread_sequence_;
  std::array<etl::timer::id::type, SIZE> id_;
};

}  // namespace paraos

#endif /* PARAOS_THREAD_SEQUENCE_HPP */
