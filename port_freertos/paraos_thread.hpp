/// @file paraos_thread.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_THREAD_V2_HPP
#define PARAOS_THREAD_V2_HPP

#include <cstdlib>

#include "FreeRTOS.h"
#include "etl/atomic.h"
#include "etl/delegate.h"
#include "gsl/gsl"
#include "paraos_attr.h"
#include "paraos_base.hpp"
#include "paraos_critical.hpp"
#include "paraos_exceptions.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_thread_common.hpp"
#include "paraos_thread_exceptions.hpp"
#include "paraos_trace.hpp"
#include "paraos_utils.hpp"
#include "task.h"
#include "timers.h"

namespace paraos {

constexpr delay_type default_sleep_ms_if_no_delegate_{700};

class Thread : public paraos::Base {
 public:
  /// @brief Construct a new Thread object.
  ///
  /// @param[in] attr: Params to initialize thread.
  /// @param[in] thread_start_flag: Flag that indicates thread start condition.
  /// May be useful in tests where there is no multithread environment needed.
  ///
  /// @throw Can throw "thread_not_created_exception".
  explicit Thread(const paraos::ThreadAttr &attr, bool thread_start_flag = true)
      : paraos::Base{attr.dtor_callback} {
    // Before create the thread, register the delegate.
    RegisterDelegate(attr.run_);

    if (thread_start_flag) {
      Make(attr);
    }
  }

  /// --------------------------------------------------------------------------

  ~Thread() override {
    paraosTRACE_MESSAGE_WITH_ACTOR_NAME("~Thread", GiveName());

    // Try delete the task atomically.
    const paraos::CriticalSection critical;

    // If scheduler is not started, calls vTaskDelete() is illegal.
    if (handle_ != nullptr && IsSchedulerRunning()) {
      vTaskDelete(handle_);
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
  /// paraos::ThreadAttr or registered later using RegisterDelegate().
  ///
  /// @see https://www.etlcpp.com/delegate.html to delegate creation examples.
  void RegisterDelegate(paraos::thread_delegate_type run) noexcept {
    // std::move of the variable of a trivially-copyable type has no effect
    run_ = run;
  }

  /// --------------------------------------------------------------------------

  /// @brief Set the priority to the thread.
  ///
  /// @param[in] priority: The priority to which the thread will be set.
  auto SetPriority(paraos::ThreadPriority priority) noexcept {
    bool result{false};

    if (priority < paraos::ThreadPriority::kMaxNum) {
      vTaskPrioritySet(handle_, static_cast<UBaseType_t>(priority));
      result = true;
    }

    return result;
  }

  /// --------------------------------------------------------------------------

  /// @brief Obtain the priority of the thread.
  ///
  /// @return paraos::ThreadPriority.
  auto GetPriority() noexcept {
    return static_cast<paraos::ThreadPriority>(uxTaskPriorityGet(handle_));
  }

  /// --------------------------------------------------------------------------

  /// @brief Obtain the thread name.
  ///
  /// @return std::string_view.
  [[nodiscard]] auto GiveName() const noexcept -> std::string_view {
    TaskStatus_t xTaskDetails;
    vTaskGetInfo(handle_, &xTaskDetails, pdFALSE, eInvalid);

    return std::string_view{xTaskDetails.pcTaskName};
  }

  /// --------------------------------------------------------------------------

  /// @brief Delay a task for a given number of milliseconds.
  ///
  /// @param[in] sleep_ms: The amount of time, that the calling thead should
  /// block.
  static void DelayMs(paraos::delay_type sleep_ms) noexcept {
    vTaskDelay(PARAOS_ConvertMsToTicks(sleep_ms));
  }

  /// --------------------------------------------------------------------------

  /// @brief Calls this method to complete the thread's work.
  ///
  /// @param[in] deferred Pointer to the object that will be deleted
  /// when the thread completes its work. Use the address only if `deferred`
  /// has been created on the heap and is not managed by user code or smart
  /// pointers.
  void Finished(paraos::Base *deferred = nullptr) noexcept {
    base_ = deferred;
    UnregisterDelegate();
  }

  /// --------------------------------------------------------------------------

  /// @brief StartScheduler() must run in the main thread.
  /// The created threads start executing only after the user code calls
  /// StartScheduler().
  static void StartScheduler() { vTaskStartScheduler(); }

  /// --------------------------------------------------------------------------

  /// @brief To resume calls Exit().
  static void DeleteAll() {}

  /// --------------------------------------------------------------------------

  /// @brief Call the method if you ready to exit from program.
  static void Exit() {
    paraosTRACE_MESSAGE_WITH_ACTOR_NAME("Call Exit()", "");
    vTaskEndScheduler();
  }

 private:
  /// @brief Create the thread with attr params.
  ///
  /// @param[in] attr: Params to initialize thread.
  ///
  /// @throw Can throw "thread_not_created_exception"
  void Make(const paraos::ThreadAttr &attr) {
    RegisterDelegate(attr.run_);

    xTaskCreate(
        RunThreadContext, attr.thread_name.data(),
        paraos::ConvertStackSizeInWords(attr.stack_depth), this,
        static_cast<UBaseType_t>(attr.priority), &handle_);

    ETL_ASSERT(handle_, ETL_ERROR(paraos::thread_not_created_exception));
  }

  /// --------------------------------------------------------------------------

  static void RunThreadContext(void *lpParam) {
    auto *thread = static_cast<Thread *>(lpParam);

    // Loop for thread.
    while (thread->is_need_while_) {
      if (!thread->run_.call_if()) {
        paraosTRACE_MESSAGE_WITH_ACTOR_NAME(
            "Delegate not ready yet, sleep in "
                << thread->sleep_ms_if_no_delegate_,
            thread->GiveName());
        Thread::DelayMs(thread->sleep_ms_if_no_delegate_);
      }
    }

    decltype(thread->handle_) handle;
    {
      const paraos::CriticalSection critical;

      // Copy task handle in local variable ...
      handle = thread->handle_;

      // ... then set to nullptr in the private field.
      // This is necessary to prevent the task from being deleted in the thread
      // destructor.
      thread->handle_ = nullptr;
    }

    // Now vTaskDelete(handle) uses the local copy of the task handle.
    // This means that if the thread object is destroyed early,
    // vTaskDelete(handle) will correctly delete the FreeRTOS thread.

    // Do not use a critical section!
    // If a critical section is used, it will remain open after calling
    // vTaskDelete(handle).

    // If user want to destroy the object ...
    if (thread->base_ != nullptr) {
      decltype(thread->base_) ptr_to_delete{nullptr};
      {
        const paraos::CriticalSection critical;

        ptr_to_delete = thread->base_;
        thread->base_ = nullptr;
        paraosTRACE_MESSAGE_WITH_ACTOR_NAME(
            "Thread finished, now put request to delete object",
            thread->GiveName());
      }

      delete ptr_to_delete;
    }

    if (handle != nullptr) {
      paraosTRACE_MESSAGE_WITH_ACTOR_NAME("Delete self task", "");
      vTaskDelete(handle);
    }
  }

  /// --------------------------------------------------------------------------

  void UnregisterDelegate() noexcept {
    paraosTRACE_MESSAGE_WITH_ACTOR_NAME("Thread clear delegate", GiveName());
    run_.clear();

    // No more need call user function inside thread.
    is_need_while_ = false;
    paraosTRACE_MESSAGE_WITH_ACTOR_NAME("Thread break while", GiveName());
  }

  /// --------------------------------------------------------------------------

  [[nodiscard]] static auto IsSchedulerRunning() noexcept -> bool {
    bool is_scheduler_started{false};
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
      is_scheduler_started = true;
    }

    return is_scheduler_started;
  }

  /// --------------------------------------------------------------------------

  etl::atomic_bool is_need_while_{true};

  /// @brief Task Handle. When freeRTOS will create the task, task handle will
  /// be written here.
  TaskHandle_t handle_{nullptr};

  /// @brief Run this delegate in thread context.
  paraos::thread_delegate_type run_;

  /// @brief When thread object calls dtor, it try delete base_.
  paraos::Base *base_{nullptr};

  /// @brief Until a user code doesn't call RegisterDelegate(), the thread will
  /// be sleep after each check to delegate available.
  delay_type sleep_ms_if_no_delegate_{default_sleep_ms_if_no_delegate_};
};

}  // namespace paraos
#endif /* PARAOS_THREAD_V2_HPP */
