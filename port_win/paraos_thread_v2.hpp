/// @file paraos_thread_v2.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
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
///
/// NAME
///     paraos_thread_v2.
///
/// DESCRIPTION
///     paraos_thread_v2 provides a WinAPI wrapper for working with threads.
///     This wrapper offers a cross-platform API for managing threads
///     in RTOS, Windows, and Linux.
///
/// EXAMPLE
///     See usages example in:
///     - port_tests/test_paraos_thread_only_global.cpp
///     -
///       port_tests/test_paraos_thread_only_stack_with_multiple_threads_in_one_object.cpp
///     - port_tests/test_paraos_thread_only_stack.cpp
///     - port_tests/test_paraos_thread_only_static.cpp

#ifndef PARAOS_THREAD_V2_HPP
#define PARAOS_THREAD_V2_HPP

#include <winbase.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "etl/atomic.h"
#include "etl/delegate.h"
#include "gsl/gsl"
#include "paraos_attr.h"
#include "paraos_base.hpp"
#include "paraos_exceptions.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_thread_common.hpp"
#include "paraos_thread_exceptions.hpp"
#include "paraos_trace.hpp"
#include "paraos_utils.hpp"

namespace paraos {
namespace v2 {

class Thread : public paraos::Base {
 public:
  /// @brief Construct a new Thread object.
  ///
  /// @param[in] attr: Params to initialize thread.
  ///
  /// @throw Can throw "thread_not_created_exception".
  explicit Thread(const paraos::v2::ThreadAttr &attr)
      : paraos::Base{attr.dtor_callback}, name_{attr.thread_name} {
    // Before create the thread, register the delegate.
    RegisterDelegate(attr.run_);

    Make(attr);
  }

  /// --------------------------------------------------------------------------

  ~Thread() override {
    paraosTRACE_MESSAGE_WITH_ACTOR_NAME("~Thread", GiveName());

    // With static object don't worry about correctly delete
    // the thread. For dynamic object, using only deferred delete when ~Thread()
    // calls in event loop context only after user calls Finished().
    if (handle_ != nullptr) {
      CloseHandle(handle_);
      handle_ = nullptr;
    }
  }

  /// Five rule ----------------------------------------------------------------
  Thread(const Thread &other) = delete;
  Thread(Thread &&other) = delete;
  auto operator=(const Thread &other) -> Thread & = delete;
  auto operator=(Thread &&other) -> Thread & = delete;

  /// --------------------------------------------------------------------------

  /// @brief The user code must provide a delegate to execute in the thread
  /// context. The delegate can be passed to the constructor via
  /// paraos::v2::ThreadAttr or registered later using RegisterDelegate().
  ///
  /// @see https://www.etlcpp.com/delegate.html to delegate creation examples.
  void RegisterDelegate(paraos::v2::thread_delegate_type run) {
    run_ = std::move(run);
  }

  /// --------------------------------------------------------------------------

  /// @brief Set the priority to the thread.
  ///
  /// @param[in] priority: The priority to which the thread will be set.
  auto SetPriority(const paraos::v2::ThreadPriority priority) {
    auto is_priority_updated =
        SetThreadPriority(handle_, static_cast<int>(priority));

    return static_cast<bool>(is_priority_updated);
  }

  /// --------------------------------------------------------------------------

  /// @brief Obtain the priority of the thread.
  ///
  /// @return paraos::v2::ThreadPriority.
  auto GetPriority() {
    return static_cast<paraos::v2::ThreadPriority>(GetThreadPriority(handle_));
  }

  /// --------------------------------------------------------------------------

  /// @brief Obtain the thread name.
  ///
  /// @return std::string_view.
  [[nodiscard]] auto GiveName() const -> std::string_view {
    return static_cast<std::string_view>(name_);
  }

  /// --------------------------------------------------------------------------

  /// @brief Delay a task for a given number of milliseconds.
  ///
  /// @param[in] sleep_ms: The amount of time, that the calling thead should
  /// block.
  static void DelayMs(paraos::delay_type sleep_ms) { Sleep(sleep_ms); }

  /// --------------------------------------------------------------------------

  /// @brief Calls this method to complete the thread's work.
  ///
  /// @param[in] deferred Pointer to the object that will be deleted
  /// when the thread completes its work. Use the address only if `deferred`
  /// has been created on the heap and is not managed by user code or smart
  /// pointers.
  void Finished(paraos::Base *deferred = nullptr) {
    base_ = deferred;
    UnregisterDelegate();
  }

  /// --------------------------------------------------------------------------

  /// @brief StartScheduler() must run in the main thread.
  /// The created threads start executing only after the user code calls
  /// StartScheduler().
  static void StartScheduler() {
    paraosTRACE_MESSAGE("Start Scheduler");

    {
      const paraos::CriticalSection critical;
      // Threads are created in a suspended state.
      // It is necessary to resume execution of the created threads.
      for (auto &thread : to_resume_) {
        ResumeThread(thread->handle_);
        paraosTRACE_MESSAGE_WITH_ACTOR_NAME(
            "Thread resume", thread->GiveName());
      }

      is_scheduler_started_ = true;
    }

    // After that scheduler stated, no more need getting storage thread
    // pointers. New threads will be created and start immediately.
    to_resume_.clear();
    to_resume_.shrink_to_fit();
  }

  /// --------------------------------------------------------------------------

  /// @brief To resume calls Exit().
  static void DeleteAll() {
    paraosTRACE_MESSAGE_WITH_ACTOR_NAME(
        "Wait while user calls Exit()", "DeleteAll");

    // The semaphore is released when the user code calls Exit().
    to_exit_.Take(paraos::max_delay);

    paraosTRACE_MESSAGE_WITH_ACTOR_NAME("User calls Exit()", "DeleteAll");
  }

  /// --------------------------------------------------------------------------

  /// @brief Call the method if you ready to exit from program.
  static void Exit() { to_exit_.Give(); }

 private:
  /// @brief Create the thread with attr params.
  ///
  /// @param[in] attr: Params to initialize thread.
  ///
  /// @throw Can throw "thread_not_created_exception"
  void Make(const paraos::v2::ThreadAttr &attr) {
    DWORD creation_flags{CREATE_SUSPENDED};

    const paraos::CriticalSection critical;

    // After the scheduler starts, there is no need to create threads in a
    // suspended state.
    if (is_scheduler_started_) {
      creation_flags = 0;
    }

    handle_ = CreateThread(
        nullptr,                         // default security attributes
        attr.stack_depth,                // use default stack size
        RunThreadContext,                // thread function name
        reinterpret_cast<LPVOID>(this),  // argument to thread function
        creation_flags,                  // use default creation flags
        nullptr);                        // returns the thread identifier

    ETL_ASSERT(handle_, ETL_ERROR(paraos::thread_not_created_exception));

    if (!is_scheduler_started_) {
      // Push only threads that's must be resume when a user code calls
      // StartScheduler().
      to_resume_.push_back(this);
    }
  }

  /// --------------------------------------------------------------------------

  static auto WINAPI RunThreadContext(LPVOID lpParam) -> DWORD {
    auto *thread = reinterpret_cast<Thread *>(lpParam);

    while (thread->is_need_while_) {
      if (!thread->run_.call_if()) {
        paraosTRACE_MESSAGE_WITH_ACTOR_NAME(
            "Delegate not ready yet, sleep in "
                << thread->sleep_ms_if_no_delegate_,
            thread->GiveName());
        Thread::DelayMs(thread->sleep_ms_if_no_delegate_);
      }
    }

    // Free resources if need will make after return operator.
    auto free_resourse_after_callback = gsl::finally([&] {
      paraosTRACE_MESSAGE_WITH_ACTOR_NAME(
          "Thread finished, now it is calls 'delete' operator to 'base_' "
          "object",
          thread->GiveName());
      /// Operator delete can invoke ~Thread(). Using lambda for calls after
      /// 'return static_cast<DWORD>(0)'.
      delete thread->base_;
    });

    return static_cast<DWORD>(0);
  }

  /// --------------------------------------------------------------------------

  void UnregisterDelegate() {
    paraosTRACE_MESSAGE_WITH_ACTOR_NAME("Thread clear delegate", GiveName());
    run_.clear();

    // No more need call user function inside thread.
    is_need_while_ = false;
    paraosTRACE_MESSAGE_WITH_ACTOR_NAME("Thread break while", GiveName());
  }

  /// --------------------------------------------------------------------------

  static inline etl::atomic_bool is_scheduler_started_{false};
  static inline std::vector<paraos::v2::Thread *> to_resume_;

  /// @brief DeleteAll() returns control only when the user code calls Exit().
  /// This is necessary to ensure a smooth process completion and to prevent
  /// Valgrind warnings.
  ///
  /// @note
  /// - DeleteAll() waits for the semaphore indefinitely.
  /// - Exit() releases the semaphore.
  static inline paraos::SemaphoreBinary to_exit_;

  /// @brief Thread name. To read the field, use GiveName().
  std::string name_;

  /// @brief While true, the delegate is called in an infinite loop.
  /// When the user code calls Finish(), this field is set to false, breaking
  /// the loop.
  etl::atomic_bool is_need_while_{true};

  HANDLE handle_{nullptr};

  /// @brief Until the user code calls RegisterDelegate(), the thread will
  /// sleep after each check for delegate availability.
  delay_type sleep_ms_if_no_delegate_{700};

  /// @brief Run this delegate in thread context.
  paraos::v2::thread_delegate_type run_;

  /// @brief The user code can provide a pointer to an object that should be
  /// destroyed after the thread completes its work when Finish() is called.
  /// This pointer stores a reference to the destroyable object, ensuring safe
  /// deletion from the heap after the thread finishes execution.
  paraos::Base *base_{nullptr};
};

}  // namespace v2
}  // namespace paraos

#endif /* PARAOS_THREAD_V2_HPP */
