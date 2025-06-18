/// @file paraos_oneshot_executor.hpp
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
///
/// @copyright (c) 2025 Stilsoft
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

#ifndef PARAOS_ONESHOT_EXECUTOR_HPP
#define PARAOS_ONESHOT_EXECUTOR_HPP

#include "etl/delegate.h"
#include "paraos_critical.hpp"
#include "paraos_queue_blocking.hpp"
#include "paraos_thread.hpp"

namespace paraos {

/// @brief Delegates type used in executor.
using executor_delegate_type = etl::delegate<void(void)>;

/// @brief Default executor queue size.
inline constexpr size_t DEFAULT_EXECUTOR_QUEUE_SIZE{10};

/// @brief Attributes to initialize IOneShotExecutor.
struct IOneShotExecutorAttributes : public paraos::ThreadAttr {};

/// @brief Interface that implements class whose job is to execute delegates
/// from it's queue once.
class IOneShotExecutor {
 public:
  /// @brief Method is used to place delegate into executor's queue.
  ///
  /// @param[in] delegate: Delegate that needs to be executed.
  ///
  /// @return Returns true in case of successful delegate emplacing, otherwise
  /// returns false.
  auto EnqueueDelegate(executor_delegate_type delegate) {
    return queue_.TryPush(delegate);
  }

  /// @brief Method is used to place delegate into executor's queue using object
  /// and method.
  ///
  /// @tparam T Type of the object containing the method.
  /// @tparam Method Pointer to the method to be called (must be void(void)).
  ///
  /// @param[in] instance Reference to the object instance.
  ///
  /// @return Returns true in case of successful delegate emplacing, otherwise
  /// returns false.
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
  auto EnqueueDelegate(T& instance) {
    auto delegate = paraos::executor_delegate_type::create<T, Method>(instance);
    return queue_.TryPush(delegate);
  }

  /// @brief Method describes one IOneShotExecutor thread iteration.
  void ExecuteDelegates() {
    auto delegate_opt = queue_.Pop(paraos::max_delay);

    if (delegate_opt) {
      delegate_opt->call_if();
    }
  }

  /// @brief Method is used to finish executor thread.
  void Finish() {
    thread_.Finished();

    // Enqueue empty delegate to unblock executor thread.
    const executor_delegate_type empty_delegate;
    EnqueueDelegate(empty_delegate);
  }

  virtual ~IOneShotExecutor() = default;

  IOneShotExecutor(IOneShotExecutor&& other) = delete;
  auto operator=(IOneShotExecutor&& other) -> IOneShotExecutor& = delete;
  auto operator=(const IOneShotExecutor& other) -> IOneShotExecutor& = delete;
  IOneShotExecutor(const IOneShotExecutor& other) = delete;

 protected:
  /// @brief IOneShotExecutor constructor.
  ///
  /// @param[in] attrs: Class attributes needed for it's initialization.
  /// @param[in] queue: Address of the blocking queue interface.
  /// @param[in] thread_start_flag: Flag indicating whether to start the thread
  /// immediately. Useful in test environments without multithreading.
  IOneShotExecutor(
      const IOneShotExecutorAttributes& attrs,
      paraos::IQueueBlocking<executor_delegate_type>& queue,
      bool thread_start_flag = true)
      : thread_{attrs, thread_start_flag}, queue_{queue} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<
            IOneShotExecutor, &IOneShotExecutor::ExecuteDelegates>(*this));
  }

 private:
  paraos::Thread thread_;

  paraos::IQueueBlocking<executor_delegate_type>& queue_;
};

/// @brief Attributes for OneShotExecutor initialization.
struct OneShotExecutorAttributes : public IOneShotExecutorAttributes {};

/// @brief Realization of OneShotExecutor interface that stores delegates queue
/// size.
///
/// @tparam QUEUE_SIZE Maximum number of delegates that can be stored in
/// blocking queue.
template <size_t QUEUE_SIZE = DEFAULT_EXECUTOR_QUEUE_SIZE>
class OneShotExecutor : public IOneShotExecutor {
 public:
  /// @brief Constructor of the OneShotExecutor class.
  ///
  /// @param[in] attrs: Attributes to initialize OneShotExecutor class.
  /// @param[in] thread_start_flag: Flag indicating whether to start the thread
  /// immediately. Useful in test environments without multithreading.
  explicit OneShotExecutor(
      const OneShotExecutorAttributes& attrs, bool thread_start_flag = true)
      : IOneShotExecutor{attrs, queue_, thread_start_flag} {}

  ~OneShotExecutor() override = default;

  OneShotExecutor(OneShotExecutor&& other) = delete;
  auto operator=(OneShotExecutor&& other) -> OneShotExecutor& = delete;
  auto operator=(const OneShotExecutor& other) -> OneShotExecutor& = delete;
  OneShotExecutor(const OneShotExecutor& other) = delete;

 private:
  paraos::QueueBlocking<executor_delegate_type, QUEUE_SIZE> queue_;
};

}  // namespace paraos

#endif /* PARAOS_ONESHOT_EXECUTOR_HPP */
