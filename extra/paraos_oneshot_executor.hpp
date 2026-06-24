/// @file paraos_oneshot_executor.hpp
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_ONESHOT_EXECUTOR_HPP
#define PARAOS_ONESHOT_EXECUTOR_HPP

#include <optional>

#include "etl/delegate.h"
#include "paraos_critical.hpp"
#include "paraos_jthread.hpp"
#include "paraos_queue_blocking.hpp"

namespace paraos {

/// @brief Type alias for delegate used in executor operations.
using executor_delegate_type = etl::delegate<void(void)>;

/// @brief Default capacity of the executor's delegate queue.
inline constexpr size_t DEFAULT_EXECUTOR_QUEUE_SIZE{10};

/// @brief Attributes structure for initializing IOneShotExecutor.
///
/// @note Inherits thread attributes from ThreadAttr.
struct IOneShotExecutorAttributes : public paraos::ThreadAttr {};

/// @brief Interface for a single-shot executor that processes delegates from a
/// queue.
///
/// This interface provides mechanisms to enqueue delegates for asynchronous
/// execution and manage the executor thread lifecycle.
///
/// @tparam QUEUE_SIZE Maximum number of delegates that can be stored in the
/// queue.
template <size_t QUEUE_SIZE = DEFAULT_EXECUTOR_QUEUE_SIZE>
class IOneShotExecutor {
 public:
  /// @brief Adds a delegate to the executor's queue for execution.
  ///
  /// @param[in] delegate Delegate to be executed.
  /// @param[in] is_isr Flag indicating if called from an ISR context.
  ///
  /// @return True if the delegate was successfully enqueued, false otherwise.
  auto EnqueueDelegate(executor_delegate_type delegate, bool is_isr = false) {
    return queue_.TryPush(delegate, is_isr);
  }

  /// @brief Adds a function as a delegate to the executor's queue.
  ///
  /// @tparam Function Pointer to a free function (must be void(void)).
  /// @param[in] is_isr Flag indicating if called from an ISR context.
  ///
  /// @return True if the delegate was successfully enqueued, false otherwise.
  ///
  /// @note Automatically wraps the function in a delegate.
  ///
  /// @example
  /// @code
  /// void MyFunction() { /* ... */ };
  /// executor.EnqueueDelegate<MyFunction>();
  /// @endcode
  template <void (*Function)(void)>
  auto EnqueueDelegate(bool is_isr = false) {
    auto delegate = paraos::executor_delegate_type::create<Function>();
    return queue_.TryPush(delegate, is_isr);
  }

  /// @brief Adds a method of an object as a delegate to the executor's queue.
  ///
  /// @tparam T Type of the object containing the method.
  /// @tparam Method Pointer to a method (must be void(void)).
  /// @param[in] instance Reference to the object instance.
  /// @param[in] is_isr Flag indicating if called from an ISR context.
  ///
  /// @return True if the delegate was successfully enqueued, false otherwise.
  ///
  /// @note Automatically wraps the method in a delegate.
  ///
  /// @example
  /// @code
  /// class MyClass {
  ///   void MyMethod() { /* ... */ }
  /// };
  /// MyClass obj;
  /// executor.EnqueueDelegate<MyClass, &MyClass::MyMethod>(obj);
  /// @endcode
  template <typename T, void (T::*Method)(void)>
  auto EnqueueDelegate(T& instance, bool is_isr = false) {
    auto delegate = paraos::executor_delegate_type::create<T, Method>(instance);
    return queue_.TryPush(delegate, is_isr);
  }

  /// @brief Terminates the executor thread gracefully.
  ///
  /// @param[in] is_isr Flag indicating if called from an ISR context.
  ///
  /// @note Requests the jthread to stop, unblocks the queue with an empty
  /// delegate, and joins the thread.
  void Finish(bool is_isr = false) {
    if (thread_.has_value()) {
      (void)thread_->request_stop();
    }

    // Enqueue empty delegate to unblock executor thread.
    const executor_delegate_type empty_delegate;
    EnqueueDelegate(empty_delegate, is_isr);

    if (thread_.has_value() && thread_->joinable()) {
      thread_->join();
    }
  }

  virtual ~IOneShotExecutor() = default;

  IOneShotExecutor(IOneShotExecutor&& other) = delete;
  auto operator=(IOneShotExecutor&& other) -> IOneShotExecutor& = delete;
  auto operator=(const IOneShotExecutor& other) -> IOneShotExecutor& = delete;
  IOneShotExecutor(const IOneShotExecutor& other) = delete;

 protected:
  /// @brief Constructor for IOneShotExecutor.
  ///
  /// @param[in] attrs Configuration attributes for the executor.
  /// @param[in] queue Reference to the blocking queue implementation.
  /// @param[in] thread_start_flag Whether to start the thread immediately
  /// (useful for testing). When `false`, no jthread is created.
  IOneShotExecutor(
      const IOneShotExecutorAttributes& attrs,
      paraos::IQueueBlocking<executor_delegate_type, QUEUE_SIZE>& queue,
      bool thread_start_flag = true)
      : queue_{queue} {
    if (thread_start_flag) {
      thread_.emplace(
          static_cast<const paraos::ThreadAttr&>(attrs),
          [this](const paraos::stop_token& token) -> void { ExecuteDelegates(token); });
    }
  }

 private:
  /// @brief Processes delegates from the queue until a stop is requested.
  ///
  /// @note This method is intended for internal use by the executor thread.
  void ExecuteDelegates(const paraos::stop_token& token) {
    while (!token.stop_requested()) {
      auto delegate_opt = queue_.Pop(paraos::max_delay);

      if (delegate_opt) {
        delegate_opt->call_if();
      }
    }
  }

  paraos::IQueueBlocking<executor_delegate_type, QUEUE_SIZE>& queue_;
  std::optional<paraos::jthread> thread_;
};

/// @brief Attributes structure for OneShotExecutor initialization.
///
/// @note Extends IOneShotExecutorAttributes with additional parameters if
/// needed.
struct OneShotExecutorAttributes : public IOneShotExecutorAttributes {};

/// @brief Concrete implementation of IOneShotExecutor with fixed-size delegate
/// queue.
///
/// @tparam QUEUE_SIZE Maximum number of delegates that can be stored in the
/// queue.
template <size_t QUEUE_SIZE = DEFAULT_EXECUTOR_QUEUE_SIZE>
class OneShotExecutor : public IOneShotExecutor<QUEUE_SIZE> {
 public:
  /// @brief Constructor for OneShotExecutor.
  ///
  /// @param[in] attrs Configuration attributes for the executor.
  /// @param[in] thread_start_flag Whether to start the thread immediately
  /// (useful for testing).
  explicit OneShotExecutor(
      const OneShotExecutorAttributes& attrs, bool thread_start_flag = true)
      : IOneShotExecutor<QUEUE_SIZE>{attrs, queue_, thread_start_flag} {}

  ~OneShotExecutor() override = default;

  OneShotExecutor(OneShotExecutor&& other) = delete;
  auto operator=(OneShotExecutor&& other) -> OneShotExecutor& = delete;
  auto operator=(const OneShotExecutor& other) -> OneShotExecutor& = delete;
  OneShotExecutor(const OneShotExecutor& other) = delete;

 private:
  /// @brief Internal delegate queue with fixed capacity.
  paraos::QueueBlocking<executor_delegate_type, QUEUE_SIZE> queue_;
};

}  // namespace paraos

#endif /* PARAOS_ONESHOT_EXECUTOR_HPP */
