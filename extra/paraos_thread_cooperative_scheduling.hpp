/// @file paraos_thread_cooperative_scheduling.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_THREAD_COOPERATIVE_SCHEDULING_HPP
#define PARAOS_THREAD_COOPERATIVE_SCHEDULING_HPP

#include <cstdlib>
#include <optional>

#include "etl/delegate.h"
#include "etl/function.h"
#include "etl/scheduler.h"
#include "etl/task.h"
#include "paraos_base.hpp"
#include "paraos_critical.hpp"
#include "paraos_jthread.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_sleep.hpp"
#include "paraos_trace.hpp"

namespace paraos {

/// @brief Amount of time in milliseconds the cooperative scheduler sleeps
/// inside its `Run()` method.
constexpr size_t coop_scheduler_delay_ms{1000};

// =============================================================================
// Scheduling policies.
// =============================================================================

/// @brief This policy runs all tasks in the list sequentially, regardless of
/// whether a task has pending work.
///
/// @note This may be useful when all tasks in the list need to be executed
/// at the start of a new tick in a cooperative scheduler.
///
/// @author Simakov Matvey.
struct cooperative_scheduler_policy_run_all_at_once {
  /// @brief Schedules all tasks in the list sequentially.
  ///
  /// @param[in] task_list: List of tasks to be scheduled.
  /// @return Always returns `true` to indicate that the scheduler should call
  /// the idle callback method.
  static auto schedule_tasks(etl::ivector<etl::task *> &task_list) -> bool {
    for (auto &scheduled_task : task_list) {
      etl::task &task = *scheduled_task;
      task.task_process_work();
    }
    // Always return true to indicate that the scheduler should call the idle
    // callback method.
    return true;
  }
};

// =============================================================================
// Cooperative scheduler realization.
// =============================================================================

/// @brief Parameters to pass to `ICooperativeScheduling` constructor.
struct ICooperativeSchedulingAttr : public paraos::ThreadAttr {
  IEmbeddedTimer *embedded_timer_ptr = &embedded_timer_empty;
};

class ICooperativeScheduling : public paraos::Base {
  using idle_delegate = etl::delegate<void()>;

 protected:
  /// @brief Constructs a new `ICooperativeScheduling` object.
  ///
  /// @param[in] attr: Attributes to initialize the thread.
  /// @param[in] scheduler: Reference to the scheduler.
  /// @param[in] thread_start_flag: Flag indicating whether to start the thread
  /// immediately. Useful in test environments without multithreading.
  ///
  /// @throw Can throw `thread_not_created_exception`.
  // NOLINTBEGIN(performance-unnecessary-value-param)
  ICooperativeScheduling(
      const ICooperativeSchedulingAttr &attr, etl::ischeduler &scheduler,
      bool thread_start_flag = true)
      : scheduler_{scheduler},
        idle_callback(*this, &ICooperativeScheduling::Idle) {
    // `scheduler_` will call all registered tasks while they have work.
    // Only when all registered tasks complete their work, `scheduler_` will
    // call the idle function. Here, the registered idle function takes a
    // semaphore and waits for a new program cycle.
    SetIdleCallback(idle_callback);
    // Connect embedded timers for each profiler used in
    // `ICooperativeScheduling`.
    profiler_.period_.SetEmbeddedTimer(*attr.embedded_timer_ptr);
    profiler_.runtime_.SetEmbeddedTimer(*attr.embedded_timer_ptr);

    if (thread_start_flag) {
      thread_.emplace(
          static_cast<const paraos::ThreadAttr &>(attr),
          [this](const paraos::stop_token &token) -> void { Run(token); });
    }
  }
  // NOLINTEND(performance-unnecessary-value-param)

 public:
  ~ICooperativeScheduling() override { Finish(); }

  /// @brief Notifies the cooperative scheduler to start a new scheduling cycle.
  ///
  /// @param[in] is_isr: Set to `true` if called from an interrupt service
  /// routine.
  ///
  /// @return Returns `true` if the notification was successfully given.
  auto NotifyGive(const bool is_isr = false) {
    auto is_notify_given = new_cycle_ready_sem_.Give(is_isr);
    paraos::ProfilerPeriodRAII(profiler_.period_);
    // Start runtime profiling. Complete runtime when `Idle()` is called.
    profiler_.runtime_.Start();
    return is_notify_given;
  }

  /// @brief Stops task execution in the cooperative scheduler.
  ///
  /// @note Useful in unit tests when an exit from the cooperative scheduler is
  /// needed.
  ///
  /// @param[in] is_dynamic: Kept for API compatibility; ignored. The jthread
  /// destructor handles cleanup.
  void Finish(bool is_dynamic = false) {
    (void)is_dynamic;

    // Request the scheduler thread to stop.
    if (thread_.has_value()) {
      (void)thread_->request_stop();
    }

    // Cause `scheduler_.start()` to return.
    scheduler_.exit_scheduler();

    // Force give notify to unblock `Idle()` in case the scheduler thread is
    // waiting for a new cycle.
    NotifyGive();

    // Wait for the scheduler thread to finish gracefully.
    if (thread_.has_value() && thread_->joinable()) {
      thread_->join();
    }
  }

  /// @brief Adds a task to the execution list, which runs when `Run()` is
  /// called. The task's position in the list depends on its priority, which is
  /// set in the task constructor. Tasks with higher priority will be executed
  /// first in each scheduler iteration.
  ///
  /// @param[in] task: The task to be added to the private list.
  /// @return Returns `true` if the task was successfully added, `false`
  /// otherwise.
  virtual auto AddTask(etl::task &task) -> bool {
    bool is_task_add{false};
    try {
      const paraos::CriticalSection critical;
      scheduler_.add_task(task);
      is_task_add = true;
    } catch (const etl::scheduler_too_many_tasks_exception &e) {
      paraosTRACE_MESSAGE(
          e.file_name() << "; --line: " << e.line_number()
                        << "; --what: " << e.what());
    }
    return is_task_add;
  }

  /// @brief Sets a custom idle function to be called by the scheduler when
  /// there is no more work in the current iteration.
  ///
  /// @param[in] callback: User-defined function that is called after all tasks
  /// have completed their work.
  void SetIdleCallback(etl::ifunction<void> &callback) {
    const paraos::CriticalSection critical;
    scheduler_.set_idle_callback(callback);
  }

  /// @brief Main loop function executed by the thread.
  /// Runs until a stop is requested or the scheduler is exited.
  void Run(const paraos::stop_token &token) {
    if (token.stop_requested()) {
      return;
    }

    try {
      scheduler_.start();
    } catch (etl::scheduler_no_tasks_exception &e) {
      paraosTRACE_MESSAGE(
          e.file_name() << "; --line: " << e.line_number()
                        << "; --what: " << e.what());
    } catch (etl::exception &e) {
      paraosTRACE_MESSAGE(
          e.file_name() << "; --line: " << e.line_number()
                        << "; --what: " << e.what());
    }
    // Yield processor time when there are no tasks to execute. Once a task is
    // registered, `scheduler_.start()` runs in its internal loop and blocks
    // the `Idle()` method by taking a semaphore.
    paraos::sleep_for(std::chrono::milliseconds(coop_scheduler_delay_ms));
  }

  /// @brief Returns a reference to the scheduler.
  auto GetScheduler() -> etl::ischeduler & { return scheduler_; }

  /// @brief Deleted move constructor and assignment operators to enforce
  /// non-copyable and non-movable semantics.
  ICooperativeScheduling(ICooperativeScheduling &&other) = delete;
  auto operator=(ICooperativeScheduling &&other)
      -> ICooperativeScheduling & = delete;
  auto operator=(const ICooperativeScheduling &other)
      -> ICooperativeScheduling & = delete;
  ICooperativeScheduling(const ICooperativeScheduling &other) = delete;

 private:
  /// @brief Called by the scheduler when all tasks have completed their work.
  /// Waits for a new program cycle and initiates the next scheduling step.
  void Idle() {
    // Start runtime profiling when `NotifyGive()` is called and complete it
    // here.
    profiler_.runtime_.Stop();
    // After all work is completed, `scheduler_` calls the idle implementation
    // (see `SetIdleCallback()`). Here, it takes a semaphore and waits for the
    // next program cycle.
    new_cycle_ready_sem_.Take(paraos::max_delay);
  }

 private:
  /// Scheduler reference.
  etl::ischeduler &scheduler_;

  /// Binary semaphore for synchronization.
  SemaphoreBinary new_cycle_ready_sem_;

  /// @brief Member function object, needed to register the `Idle()` method in
  /// the scheduler.
  etl::function<ICooperativeScheduling, void> idle_callback;

  /// Thread instance.
  std::optional<paraos::jthread> thread_;

  struct {
    TimerProfiler period_;   ///< Profiler for measuring the period.
    TimerProfiler runtime_;  ///< Profiler for measuring runtime.
  } profiler_;
};

/// @brief Parameters to pass to `CooperativeScheduling` constructor.
struct CooperativeSchedulingAttr : public ICooperativeSchedulingAttr {};

/// @brief Constructs a cooperative scheduler.
///
/// @warning `etl::scheduler<TSchedulerPolicy, MAX_TASKS_>` must only be used
/// as a base class because `etl::scheduler` must be fully constructed
/// before the `ICooperativeScheduling` constructor is called.
///
/// @tparam MAX_TASKS_ The maximum number of tasks that can be contained at a
/// time.
/// @tparam TSchedulerPolicy The policy used for executing registered tasks.
template <
    std::size_t MAX_TASKS_,
    typename TSchedulerPolicy = etl::scheduler_policy_sequential_single>
// Intentional multiple inheritance: CooperativeScheduling combines the ETL
// scheduler implementation with the PARAOS cooperative-scheduling interface.
// This is a documented false positive for clang-tidy's
// misc-multiple-inheritance check.
// NOLINTBEGIN(misc-multiple-inheritance)
class CooperativeScheduling
    : public etl::scheduler<TSchedulerPolicy, MAX_TASKS_>,
      public ICooperativeScheduling {
 public:
  /// @brief Constructs a new `CooperativeScheduling` object.
  ///
  /// @param[in] attr: Attributes to initialize the thread.
  /// @param[in] thread_start_flag: Flag indicating whether to start the thread
  /// immediately. Useful in test environments without multithreading.
  explicit CooperativeScheduling(
      const CooperativeSchedulingAttr &attr, bool thread_start_flag = true)
      : ICooperativeScheduling{attr, *this, thread_start_flag} {}

  /// @brief Deleted move constructor and assignment operators to enforce
  /// non-copyable and non-movable semantics.
  CooperativeScheduling(CooperativeScheduling &&other) = delete;
  auto operator=(CooperativeScheduling &&other)
      -> CooperativeScheduling & = delete;
  auto operator=(const CooperativeScheduling &other)
      -> CooperativeScheduling & = delete;
  CooperativeScheduling(const CooperativeScheduling &other) = delete;

  ~CooperativeScheduling() override = default;

  // Do not use `etl::scheduler` as a private field. In this case, the
  // `ICooperativeScheduling()` constructor will be called before
  // `etl::scheduler` is fully constructed.
};
// NOLINTEND(misc-multiple-inheritance)

}  // namespace paraos

#endif /* PARAOS_THREAD_COOPERATIVE_SCHEDULING_HPP */