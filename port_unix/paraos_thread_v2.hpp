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
///     paraos_thread_v2 provides a POSIX wrapper for working with threads.
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

#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "etl/atomic.h"
#include "etl/delegate.h"
#include "gsl/gsl"
#include "paraos_attr.h"
#include "paraos_base.hpp"
#include "paraos_exceptions.hpp"
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
      : paraos::Base(attr.dtor_callback), name_{attr.thread_name} {
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
  auto SetPriority(const paraos::v2::ThreadPriority priority) -> bool {
    bool is_priority_updated{false};

    PARAOS_CHECK_ASSERT(
        IsPriorityInRange(priority) == true &&
        "Priority out of range, use only ThreadPriority definitions for change "
        "priority");

    const paraos::CriticalSection critical;

    // Changing the thread priority is only possible if the program is run with
    // superuser privileges.
    if (IsRunAsRoot()) {
      int policy{0};
      sched_param sched{};
      if (pthread_getschedparam(handle_, &policy, &sched) != 0) {
        PARAOS_CHECK_ASSERT(
            false && "pthread_getschedparam() return error code");
      }

      sched.sched_priority = static_cast<int>(priority);
      auto prior_update_status =
          pthread_setschedparam(handle_, SCHED_RR, &sched);

      paraosTRACE_MESSAGE(
          "pthread_setschedparam return core: " << prior_update_status);

      if (prior_update_status == 0) {
        is_priority_updated = true;
      }
    } else {
      // If the program is launched without superuser privileges,
      // we cannot change the thread priority.
      // In this case, we return `true` to ensure backward compatibility.
      is_priority_updated = true;
    }

    return is_priority_updated;
  }

  /// --------------------------------------------------------------------------

  /// @brief Obtain the priority of the thread.
  ///
  /// @return paraos::v2::ThreadPriority.
  auto GetPriority() {
    struct sched_param param;
    int policy{};
    int ret = pthread_getschedparam(handle_, &policy, &param);
    PARAOS_CHECK_ASSERT(ret == 0);
    PARAOS_ATTR_UNUSED_VAR(ret);
    return static_cast<paraos::v2::ThreadPriority>(param.sched_priority);
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
  static void DelayMs(std::size_t sleep_ms) {
    usleep(sleep_ms * MICROSECONDS_PER_MILISECONDS);
  }

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
    const paraos::CriticalSection critical;

    // Give semaphore for each thread to resume perform_work() execution.
    for (auto &thread : to_resume_) {
      paraosTRACE_MESSAGE(
          "Resumed thread name is: '" << thread->GiveName() << "'");
      thread->sem_.Give();
    }

    // After that scheduler stated, no more need getting storage thread
    // pointers. New threads will be created and start immediately.
    to_resume_.clear();
    to_resume_.shrink_to_fit();
  }

  /// --------------------------------------------------------------------------

  /// @brief To resume calls Exit().
  static void DeleteAll() { to_exit_.Take(); }

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
    PARAOS_CHECK_ASSERT(
        (IsPriorityInRange(attr.priority) == true) &&
        "Priority out of range, use only ThreadPriority definitions for change "
        "priority");

    int result_code{-1};

    pthread_attr_t thread_attr;

    // Lambda will be call after Make complete work. If exception will be throw,
    // pthread_attr_destroy will be call.
    auto free_resourse =
        gsl::finally([&] { pthread_attr_destroy(&thread_attr); });

    result_code = pthread_attr_init(&thread_attr);
    ETL_ASSERT(
        result_code == 0, ETL_ERROR(paraos::thread_not_created_exception));

    result_code = pthread_attr_setschedpolicy(&thread_attr, sch_policy);
    ETL_ASSERT(
        result_code == 0, ETL_ERROR(paraos::thread_not_created_exception));

    result_code =
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    ETL_ASSERT(
        result_code == 0, ETL_ERROR(paraos::thread_not_created_exception));

    // Set thread priority.
    struct sched_param param{};
    param.sched_priority = static_cast<int>(attr.priority);
    result_code = pthread_attr_setschedparam(&thread_attr, &param);
    ETL_ASSERT(
        result_code == 0, ETL_ERROR(paraos::thread_not_created_exception));

    result_code = pthread_create(&handle_, &thread_attr, perform_work, this);

    ETL_ASSERT(
        result_code == 0, ETL_ERROR(paraos::thread_not_created_exception));

    to_resume_.push_back(this);
  }

  /// --------------------------------------------------------------------------

  static auto perform_work(void *arguments) -> void * {
    auto *thread = reinterpret_cast<paraos::v2::Thread *>(arguments);

    // Need call StartScheduler() for give this semaphore.
    thread->sem_.Take(paraos::max_delay);

    while (thread->is_need_while_) {
      if (!thread->run_.call_if()) {
        paraosTRACE_MESSAGE_WITH_ACTOR_NAME(
            "Delegate not ready yet, sleep in "
                << thread->sleep_ms_if_no_delegate_,
            thread->GiveName());
        Thread::DelayMs(thread->sleep_ms_if_no_delegate_);
      }
    }

    // free_resourse_after_callback lambda will be calls after return operator
    // and call_callback lambda.
    auto free_resourse_after_callback = gsl::finally([&] {
      paraosTRACE_MESSAGE_WITH_ACTOR_NAME(
          "Thread finished, now it is calls 'delete' operator to 'base_' "
          "object",
          thread->GiveName());
      delete thread->base_;
    });

    return nullptr;
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

  [[nodiscard]] auto IsPriorityInRange(
      paraos::v2::ThreadPriority priority) const -> bool {
    bool is_in_range{false};

    auto min = sched_get_priority_min(sch_policy);
    auto max = sched_get_priority_max(sch_policy);
    auto prior = static_cast<int>(priority);

    if ((prior >= min) && (prior <= max)) {
      is_in_range = true;
    }

    return is_in_range;
  }

  /// --------------------------------------------------------------------------

  /// @brief This method checks whether the program is running
  /// with superuser privileges.
  ///
  /// @note
  /// https://stackoverflow.com/questions/3214297/how-can-my-c-c-application-determine-if-the-root-user-is-executing-the-command
  ///
  /// @return Returns true if the program is running as root, false otherwise.
  static auto IsRunAsRoot() -> bool {
    bool is_run_as_root{false};

    auto user = getuid();
    if (user == 0) {
      paraosTRACE_MESSAGE("Program run as root");
      is_run_as_root = true;
    } else {
      paraosTRACE_MESSAGE("No root");
    }
    return is_run_as_root;
  }

  static inline std::vector<paraos::v2::Thread *> to_resume_;

  /// @brief DeleteAll() returns control only when the user code calls Exit().
  /// This is necessary to ensure a smooth process completion and to prevent
  /// Valgrind warnings.
  ///
  /// @note
  /// - DeleteAll() waits for the semaphore indefinitely.
  /// - Exit() releases the semaphore.
  static inline paraos::SemaphoreBinary to_exit_;

  /// @brief Scheduling policy for all created threads (Round Robin).
  static constexpr int sch_policy{SCHED_RR};

  /// @brief Thread name. To read the field, use GiveName().
  std::string name_;

  /// @brief While true, the delegate is called in an infinite loop.
  /// When the user code calls Finish(), this field is set to false, breaking
  /// the loop.
  etl::atomic_bool is_need_while_{true};

  pthread_t handle_{0};

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

  /// @brief Semaphore used to suspend the thread until StartScheduler() is
  /// called.
  SemaphoreBinary sem_;
};

}  // namespace v2
}  // namespace paraos

#endif /* PARAOS_THREAD_V2_HPP */