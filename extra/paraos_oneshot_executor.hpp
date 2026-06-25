/// @file paraos_oneshot_executor.hpp
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_ONESHOT_EXECUTOR_HPP
#define PARAOS_ONESHOT_EXECUTOR_HPP

#include <optional>

#include "etl/delegate.h"
#include "paraos_config.hpp"
#include "paraos_critical.hpp"
#include "paraos_jthread.hpp"
#include "paraos_queue_blocking.hpp"

namespace paraos {

/// @brief Type alias for delegate used in executor operations.
using executor_delegate_type = etl::delegate<void(void)>;

/// @brief Default capacity of the executor's delegate queue.
inline constexpr size_t default_executor_queue_size{10};

/// @brief Attributes structure for initializing one_shot_executor_base.
///
/// @note Inherits thread attributes from thread_attr.
struct one_shot_executor_attr_base : public paraos::thread_attr {};

using IOneShotExecutorAttributes PARAOS_DEPRECATED(
    "use paraos::one_shot_executor_attr_base") = one_shot_executor_attr_base;

/// @brief Interface for a single-shot executor that processes delegates from a
/// queue.
///
/// This interface provides mechanisms to enqueue delegates for asynchronous
/// execution and manage the executor thread lifecycle.
///
/// @tparam QueueSize Maximum number of delegates that can be stored in the
/// queue.
template <size_t QueueSize = default_executor_queue_size>
class one_shot_executor_base {
 public:
  /// @brief Adds a delegate to the executor's queue for execution.
  ///
  /// @param[in] delegate Delegate to be executed.
  /// @param[in] is_isr Flag indicating if called from an ISR context.
  ///
  /// @return True if the delegate was successfully enqueued, false otherwise.
  auto enqueue_delegate(executor_delegate_type delegate, bool is_isr = false) {
    return queue_.try_push(delegate, is_isr);
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
  /// executor.enqueue_delegate<MyFunction>();
  /// @endcode
  template <void (*Function)(void)>
  auto enqueue_delegate(bool is_isr = false) {
    auto delegate = paraos::executor_delegate_type::create<Function>();
    return queue_.try_push(delegate, is_isr);
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
  /// executor.enqueue_delegate<MyClass, &MyClass::MyMethod>(obj);
  /// @endcode
  template <typename T, void (T::*Method)(void)>
  auto enqueue_delegate(T& instance, bool is_isr = false) {
    auto delegate = paraos::executor_delegate_type::create<T, Method>(instance);
    return queue_.try_push(delegate, is_isr);
  }

  /// @brief Terminates the executor thread gracefully.
  ///
  /// @param[in] is_isr Flag indicating if called from an ISR context.
  ///
  /// @note Requests the jthread to stop, unblocks the queue with an empty
  /// delegate, and joins the thread.
  void finish(bool is_isr = false) {
    if (thread_.has_value()) {
      (void)thread_->request_stop();
    }

    // Enqueue empty delegate to unblock executor thread.
    const executor_delegate_type empty_delegate;
    enqueue_delegate(empty_delegate, is_isr);

    if (thread_.has_value() && thread_->joinable()) {
      thread_->join();
    }
  }

  virtual ~one_shot_executor_base() = default;

  one_shot_executor_base(one_shot_executor_base&& other) = delete;
  auto operator=(one_shot_executor_base&& other)
      -> one_shot_executor_base& = delete;
  auto operator=(const one_shot_executor_base& other)
      -> one_shot_executor_base& = delete;
  one_shot_executor_base(const one_shot_executor_base& other) = delete;

  // Backward-compatible deprecated forwarding methods.
  PARAOS_DEPRECATED("use enqueue_delegate()")
  auto EnqueueDelegate(executor_delegate_type delegate, bool is_isr = false) {
    return enqueue_delegate(delegate, is_isr);
  }

  template <void (*Function)(void)>
  PARAOS_DEPRECATED("use enqueue_delegate()")
  auto EnqueueDelegate(bool is_isr = false) {
    return enqueue_delegate<Function>(is_isr);
  }

  template <typename T, void (T::*Method)(void)>
  PARAOS_DEPRECATED("use enqueue_delegate()")
  auto EnqueueDelegate(T& instance, bool is_isr = false) {
    return enqueue_delegate<T, Method>(instance, is_isr);
  }

  PARAOS_DEPRECATED("use finish()")
  void Finish(bool is_isr = false) { finish(is_isr); }

 protected:
  /// @brief Constructor for one_shot_executor_base.
  ///
  /// @param[in] attrs Configuration attributes for the executor.
  /// @param[in] queue Reference to the blocking queue implementation.
  /// @param[in] thread_start_flag Whether to start the thread immediately
  /// (useful for testing). When `false`, no jthread is created.
  one_shot_executor_base(
      const one_shot_executor_attr_base& attrs,
      paraos::queue_blocking_base<executor_delegate_type, QueueSize>& queue,
      bool thread_start_flag = true)
      : queue_{queue} {
    if (thread_start_flag) {
      thread_.emplace(
          static_cast<const paraos::thread_attr&>(attrs),
          [this](const paraos::stop_token& token) -> void {
            execute_delegates(token);
          });
    }
  }

 private:
  /// @brief Processes delegates from the queue until a stop is requested.
  ///
  /// @note This method is intended for internal use by the executor thread.
  void execute_delegates(const paraos::stop_token& token) {
    while (!token.stop_requested()) {
      auto delegate_opt = queue_.pop(paraos::max_delay);

      if (delegate_opt) {
        delegate_opt->call_if();
      }
    }
  }

  paraos::queue_blocking_base<executor_delegate_type, QueueSize>& queue_;
  std::optional<paraos::jthread> thread_;
};

template <size_t QueueSize = default_executor_queue_size>
using IOneShotExecutor PARAOS_DEPRECATED("use paraos::one_shot_executor_base") =
    one_shot_executor_base<QueueSize>;

/// @brief Attributes structure for one_shot_executor initialization.
///
/// @note Extends one_shot_executor_attr_base with additional parameters if
/// needed.
struct one_shot_executor_attr : public one_shot_executor_attr_base {};

using OneShotExecutorAttributes PARAOS_DEPRECATED(
    "use paraos::one_shot_executor_attr") = one_shot_executor_attr;

/// @brief Concrete implementation of one_shot_executor_base with fixed-size
/// delegate queue.
///
/// @tparam QueueSize Maximum number of delegates that can be stored in the
/// queue.
template <size_t QueueSize = default_executor_queue_size>
class one_shot_executor : public one_shot_executor_base<QueueSize> {
 public:
  /// @brief Constructor for one_shot_executor.
  ///
  /// @param[in] attrs Configuration attributes for the executor.
  /// @param[in] thread_start_flag Whether to start the thread immediately
  /// (useful for testing).
  explicit one_shot_executor(
      const one_shot_executor_attr& attrs, bool thread_start_flag = true)
      : one_shot_executor_base<QueueSize>{attrs, queue_, thread_start_flag} {}

  ~one_shot_executor() override = default;

  one_shot_executor(one_shot_executor&& other) = delete;
  auto operator=(one_shot_executor&& other) -> one_shot_executor& = delete;
  auto operator=(const one_shot_executor& other) -> one_shot_executor& = delete;
  one_shot_executor(const one_shot_executor& other) = delete;

 private:
  /// @brief Internal delegate queue with fixed capacity.
  paraos::queue_blocking<executor_delegate_type, QueueSize> queue_;
};

template <size_t QueueSize = default_executor_queue_size>
using OneShotExecutor PARAOS_DEPRECATED("use paraos::one_shot_executor") =
    one_shot_executor<QueueSize>;

}  // namespace paraos

#endif /* PARAOS_ONESHOT_EXECUTOR_HPP */
