/// @file paraos_thread_cooperative_scheduling.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_THREAD_COOPERATIVE_SCHEDULING_HPP
#define PARAOS_THREAD_COOPERATIVE_SCHEDULING_HPP

#include <cstdlib>
#include <optional>

#include "etl/atomic.h"
#include "etl/delegate.h"
#include "etl/function.h"
#include "etl/scheduler.h"
#include "etl/task.h"
#include "paraos_base.hpp"
#include "paraos_critical.hpp"
#include "paraos_isr.hpp"
#include "paraos_jthread.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_semaphore_std.hpp"
#include "paraos_sleep.hpp"
#include "paraos_trace.hpp"

namespace paraos {

/// @brief Amount of time in milliseconds the cooperative scheduler sleeps
/// inside its `run()` method.
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

/// @brief Parameters to pass to `cooperative_scheduling_base` constructor.
struct cooperative_scheduling_attr_base : public paraos::thread_attr {
  embedded_timer_base *embedded_timer_ptr = &embedded_timer_empty_instance;
};

using ICooperativeSchedulingAttr PARAOS_DEPRECATED(
    "use paraos::cooperative_scheduling_attr_base") =
    cooperative_scheduling_attr_base;

class cooperative_scheduling_base : public paraos::base {
  using idle_delegate = etl::delegate<void()>;

 protected:
  /// @brief Constructs a new `cooperative_scheduling_base` object.
  ///
  /// @param[in] attr: Attributes to initialize the thread.
  /// @param[in] scheduler: Reference to the scheduler.
  /// @param[in] thread_start_flag: Flag indicating whether to start the thread
  /// immediately. Useful in test environments without multithreading.
  ///
  /// @throw Can throw `thread_not_created_exception`.
  // NOLINTBEGIN(performance-unnecessary-value-param)
  cooperative_scheduling_base(
      const cooperative_scheduling_attr_base &attr, etl::ischeduler &scheduler,
      bool thread_start_flag = true)
      : scheduler_{scheduler},
        idle_callback(*this, &cooperative_scheduling_base::idle) {
    // `scheduler_` will call all registered tasks while they have work.
    // Only when all registered tasks complete their work, `scheduler_` will
    // call the idle function. Here, the registered idle function takes a
    // semaphore and waits for a new program cycle.
    set_idle_callback(idle_callback);
    // Connect embedded timers for each profiler used in
    // `cooperative_scheduling_base`.
    profiler_.period_.set_embedded_timer(*attr.embedded_timer_ptr);
    profiler_.runtime_.set_embedded_timer(*attr.embedded_timer_ptr);

    if (thread_start_flag) {
      thread_.emplace(
          static_cast<const paraos::thread_attr &>(attr),
          [this](const paraos::stop_token &token) -> void { run(token); });
    }
  }
  // NOLINTEND(performance-unnecessary-value-param)

 public:
  ~cooperative_scheduling_base() override { finish(); }

  /// @brief Notifies the cooperative scheduler to start a new scheduling cycle.
  ///
  /// @param[in] is_isr: Set to `true` if called from an interrupt service
  /// routine.
  ///
  /// @return Returns `true` if the notification was successfully given.
  auto notify_give(const bool is_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    new_cycle_ready_sem_.release();
    paraos::profiler_period_raii(profiler_.period_);
    // Start runtime profiling. Complete runtime when `idle()` is called.
    profiler_.runtime_.start();
    return paraos::isr_bool{true};
  }

  /// @brief Stops task execution in the cooperative scheduler.
  ///
  /// @note Useful in unit tests when an exit from the cooperative scheduler is
  /// needed.
  ///
  /// @param[in] is_dynamic: Kept for API compatibility; ignored. The jthread
  /// destructor handles cleanup.
  void finish(bool is_dynamic = false) {
    (void)is_dynamic;

    // Prevent double finish from destructor after explicit finish() call.
    if (is_finished_.exchange(true)) {
      return;
    }

    // Request the scheduler thread to stop.
    if (thread_.has_value()) {
      (void)thread_->request_stop();
    }

    // Cause `scheduler_.start()` to return.
    scheduler_.exit_scheduler();

    // Force give notify to unblock `idle()` in case the scheduler thread is
    // waiting for a new cycle.
    notify_give();

    // Wait for the scheduler thread to finish gracefully.
    if (thread_.has_value() && thread_->joinable()) {
      thread_->join();
    }
  }

  /// @brief Adds a task to the execution list, which runs when `run()` is
  /// called. The task's position in the list depends on its priority, which is
  /// set in the task constructor. Tasks with higher priority will be executed
  /// first in each scheduler iteration.
  ///
  /// @param[in] task: The task to be added to the private list.
  /// @return Returns `true` if the task was successfully added, `false`
  /// otherwise.
  virtual auto add_task(etl::task &task) -> bool {
    bool is_task_add{false};
    try {
      const paraos::critical_section critical;
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
  void set_idle_callback(etl::ifunction<void> &callback) {
    const paraos::critical_section critical;
    scheduler_.set_idle_callback(callback);
  }

  /// @brief Main loop function executed by the thread.
  /// Runs until a stop is requested or the scheduler is exited.
  void run(const paraos::stop_token &token) {
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
    // the `idle()` method by taking a semaphore.
    paraos::sleep_for(std::chrono::milliseconds(coop_scheduler_delay_ms));
  }

  /// @brief Returns a reference to the scheduler.
  auto scheduler() -> etl::ischeduler & { return scheduler_; }

  /// @brief Deleted move constructor and assignment operators to enforce
  /// non-copyable and non-movable semantics.
  cooperative_scheduling_base(cooperative_scheduling_base &&other) = delete;
  auto operator=(cooperative_scheduling_base &&other)
      -> cooperative_scheduling_base & = delete;
  auto operator=(const cooperative_scheduling_base &other)
      -> cooperative_scheduling_base & = delete;
  cooperative_scheduling_base(const cooperative_scheduling_base &other) =
      delete;

  // Backward-compatible deprecated forwarding methods.
  PARAOS_DEPRECATED("use notify_give()")
  auto NotifyGive(const bool is_isr = false) { return notify_give(is_isr); }

  PARAOS_DEPRECATED("use finish()")
  void Finish(bool is_dynamic = false) { finish(is_dynamic); }

  PARAOS_DEPRECATED("use set_idle_callback()")
  void SetIdleCallback(etl::ifunction<void> &callback) {
    set_idle_callback(callback);
  }

  PARAOS_DEPRECATED("use run()")
  void Run(const paraos::stop_token &token) { run(token); }

  PARAOS_DEPRECATED("use scheduler()")
  auto GetScheduler() -> etl::ischeduler & { return scheduler(); }

 private:
  /// @brief Called by the scheduler when all tasks have completed their work.
  /// Waits for a new program cycle and initiates the next scheduling step.
  void idle() {
    // Start runtime profiling when `notify_give()` is called and complete it
    // here.
    profiler_.runtime_.stop();
    // After all work is completed, `scheduler_` calls the idle implementation
    // (see `set_idle_callback()`). Here, it takes a semaphore and waits for the
    // next program cycle.
    new_cycle_ready_sem_.acquire();
  }

 private:
  /// Scheduler reference.
  etl::ischeduler &scheduler_;

  /// Binary semaphore for synchronization.
  paraos::binary_semaphore new_cycle_ready_sem_{0};

  /// @brief Member function object, needed to register the `idle()` method in
  /// the scheduler.
  etl::function<cooperative_scheduling_base, void> idle_callback;

  /// Thread instance.
  std::optional<paraos::jthread> thread_;

  /// @brief Flag to ensure finish() is executed only once.
  etl::atomic_bool is_finished_{false};

  struct {
    timer_profiler period_;   ///< Profiler for measuring the period.
    timer_profiler runtime_;  ///< Profiler for measuring runtime.
  } profiler_;
};

using ICooperativeScheduling PARAOS_DEPRECATED(
    "use paraos::cooperative_scheduling_base") = cooperative_scheduling_base;

/// @brief Parameters to pass to `cooperative_scheduling` constructor.
struct cooperative_scheduling_attr : public cooperative_scheduling_attr_base {};

using CooperativeSchedulingAttr PARAOS_DEPRECATED(
    "use paraos::cooperative_scheduling_attr") = cooperative_scheduling_attr;

/// @brief Constructs a cooperative scheduler.
///
/// @warning `etl::scheduler<TSchedulerPolicy, MAX_TASKS_>` must only be used
/// as a base class because `etl::scheduler` must be fully constructed
/// before the `cooperative_scheduling_base` constructor is called.
///
/// @tparam MaxTasks The maximum number of tasks that can be contained at a
/// time.
/// @tparam Policy The policy used for executing registered tasks.
template <
    std::size_t MaxTasks,
    typename Policy = etl::scheduler_policy_sequential_single>
// Intentional multiple inheritance: cooperative_scheduling combines the ETL
// scheduler implementation with the PARAOS cooperative-scheduling interface.
// This is a documented false positive for clang-tidy's
// misc-multiple-inheritance check.
// NOLINTBEGIN(misc-multiple-inheritance)
class cooperative_scheduling
    : public etl::scheduler<Policy, MaxTasks>,
      public cooperative_scheduling_base {
 public:
  /// @brief Constructs a new `cooperative_scheduling` object.
  ///
  /// @param[in] attr: Attributes to initialize the thread.
  /// @param[in] thread_start_flag: Flag indicating whether to start the thread
  /// immediately. Useful in test environments without multithreading.
  explicit cooperative_scheduling(
      const cooperative_scheduling_attr &attr, bool thread_start_flag = true)
      : cooperative_scheduling_base{attr, *this, thread_start_flag} {}

  // Disambiguate methods that exist in both `cooperative_scheduling_base` and
  // the `etl::scheduler` base class (e.g. `add_task`, `set_idle_callback`).
  using cooperative_scheduling_base::notify_give;
  using cooperative_scheduling_base::finish;
  using cooperative_scheduling_base::add_task;
  using cooperative_scheduling_base::set_idle_callback;
  using cooperative_scheduling_base::run;
  using cooperative_scheduling_base::scheduler;

  /// @brief Deleted move constructor and assignment operators to enforce
  /// non-copyable and non-movable semantics.
  cooperative_scheduling(cooperative_scheduling &&other) = delete;
  auto operator=(cooperative_scheduling &&other)
      -> cooperative_scheduling & = delete;
  auto operator=(const cooperative_scheduling &other)
      -> cooperative_scheduling & = delete;
  cooperative_scheduling(const cooperative_scheduling &other) = delete;

  ~cooperative_scheduling() override = default;

  // Do not use `etl::scheduler` as a private field. In this case, the
  // `cooperative_scheduling_base()` constructor will be called before
  // `etl::scheduler` is fully constructed.
};
// NOLINTEND(misc-multiple-inheritance)

template <
    std::size_t MaxTasks,
    typename Policy = etl::scheduler_policy_sequential_single>
using CooperativeScheduling PARAOS_DEPRECATED(
    "use paraos::cooperative_scheduling") = cooperative_scheduling<MaxTasks, Policy>;

}  // namespace paraos

#endif /* PARAOS_THREAD_COOPERATIVE_SCHEDULING_HPP */
