/// @file paraos_thread_cooperative_scheduling.hpp
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

#ifndef PARAOS_THREAD_COOPERATIVE_SCHEDULING_HPP
#define PARAOS_THREAD_COOPERATIVE_SCHEDULING_HPP

#include "etl/delegate.h"
#include "etl/function.h"
#include "etl/scheduler.h"
#include "etl/task.h"
#include "paraos_base.hpp"
#include "paraos_critical.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_thread.hpp"
#include "paraos_thread_v2.hpp"
#include "paraos_trace.hpp"

namespace paraos {

/// @brief Amount of time in milliseconds coop scheduler sleeps inside it's
/// "Run()" method.
inline constexpr size_t coop_scheduler_delay_ms{1000};

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
  static auto schedule_tasks(etl::ivector<etl::task *> &task_list) -> bool {
    // for (size_t index = 0UL; index < task_list.size(); ++index) {
    for (auto &scheduled_task : task_list) {
      etl::task &task = *(scheduled_task);
      task.task_process_work();
    }

    // Always return true for indicate that scheduler must call idle callback
    // method.
    return true;
  }
};

// =============================================================================
// Cooperative scheduler realization.
// =============================================================================

/// @brief Params to pass in ICooperativeScheduling{} ctor.
struct ICooperativeSchedulingAttr : public paraos::v2::ThreadAttr {
  const IEmbeddedTimer &embedded_timer_ = embedded_timer_empty;
};

class ICooperativeScheduling : public paraos::Base {
  using idle_delegate = etl::delegate<void()>;

 protected:
  /// @brief Construct a new ICooperativeScheduling object.
  ///
  /// @param[in] attr: Attributes to initialize thread.
  /// @param[in] scheduler: reference to created scheduler.
  /// @param[in] embedded_timer: reference to profiler timer.
  ///
  /// @throw Can throw "thread_not_created_exception".
  // NOLINTBEGIN(performance-unnecessary-value-param)
  ICooperativeScheduling(
      const ICooperativeSchedulingAttr &attr, etl::ischeduler &scheduler,
      const IEmbeddedTimer &embedded_timer)
      : thread_{attr},
        scheduler_{scheduler},
        idle_callback(*this, &ICooperativeScheduling::Idle) {
    thread_.RegisterDelegate(
        paraos::v2::thread_delegate_type::create<
            ICooperativeScheduling, &ICooperativeScheduling::Run>(*this));

    // scheduler_ will call all registered tasks while they have work.
    // Only when all registered tasks complete their work, scheduler_ will call
    // the idle function. Here, the registered idle function takes a semaphore
    // and waits for a new program cycle.
    SetIdleCallback(idle_callback);

    // Connect embedded timers for each profiler used in ICooperativeScheduling.
    profiler_.period_.SetEmbeddedTimer(embedded_timer);
    profiler_.runtime_.SetEmbeddedTimer(embedded_timer);
  }
  // NOLINTEND(performance-unnecessary-value-param)

 public:
  ~ICooperativeScheduling() override { Finish(); }

  /// @brief The cooperative scheduler runs periodically.
  /// This means the user code must provide periodic notifications.
  ///
  /// @param[in] is_isr Set to true if called from an interrupt.
  ///
  /// @return Returns true if the notification was successfully given.
  auto NotifyGive(const bool is_isr = false) {
    auto is_notify_given = new_cycle_ready_sem_.Give(is_isr);

    paraos::ProfilerPeriodRAII(profiler_.period_);

    // Start runtime profiling. Complete runtime when idle will calling.
    profiler_.runtime_.Start();

    return is_notify_given;
  }

  /// @brief Stops task execution in the cooperative scheduler.
  ///
  /// @note Useful in unit tests when an exit from the cooperative scheduler is
  /// needed.
  ///
  /// @param[in] is_dynamic Set to true if the cooperative scheduler was created
  /// on the heap and is not managed by user code or smart pointers.
  /// In this case, CooperativeScheduling() will be removed from the heap
  /// after the thread completes all work. Otherwise, set to false.
  void Finish(bool is_dynamic = false) {
    const paraos::CriticalSection critical;

    paraos::Base *deferred_destroy{nullptr};
    if (is_dynamic == true) {
      deferred_destroy = this;
    }

    thread_.Finished(deferred_destroy);

    scheduler_.exit_scheduler();

    // Force give notify for leave while cycle inside cooperative scheduler.
    // Need because if Exit() calls cooperative scheduler may wait notify
    // forever in Idle().
    NotifyGive();
  }

  /// @brief Adds a task to the execution list, which runs when Run() is called.
  /// The task's position in the list depends on its priority, which is set in
  /// the task constructor. This means tasks with higher priority will be
  /// executed first in each scheduler iteration.
  ///
  /// @param[in] task The task to be added to the private list. This means the
  /// task will be scheduled for execution when Run() is called in the paraos
  /// thread context.
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

  /// @brief After all tasks complete their work in one iteration, the scheduler
  /// calls the idle task. The user can set a custom idle function to be called
  /// by the scheduler when there is no more work in the current iteration.
  ///
  /// @param[in] callback User-defined function that is called after all tasks
  /// have completed their work.
  void SetIdleCallback(etl::ifunction<void> &callback) {
    const paraos::CriticalSection critical;
    scheduler_.set_idle_callback(callback);
  }

  void Run() {
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

    // The Run() method is called in a loop. When there are no tasks to execute,
    // scheduler_.start() throws an exception. After Run() catches the
    // exception, scheduler_.start() is immediately called again in an infinite
    // loop (since Run() executes in an external infinite loop). In this case,
    // all processor time would be wasted. Therefore, DelayMs() yields processor
    // time to other threads.
    //
    // Once a task is registered (when the user code calls AddTask()),
    // scheduler_.start() begins execution in an internal loop,
    // which blocks the Idle() method by taking a semaphore.
    Thread::DelayMs(coop_scheduler_delay_ms);
  }

  auto GetScheduler() -> etl::ischeduler & { return scheduler_; }

  /// @brief Five rule.
  ICooperativeScheduling(ICooperativeScheduling &&other) = delete;
  auto operator=(ICooperativeScheduling &&other)
      -> ICooperativeScheduling & = delete;
  auto operator=(const ICooperativeScheduling &other)
      -> ICooperativeScheduling & = delete;
  ICooperativeScheduling(const ICooperativeScheduling &other) = delete;

 private:
  /// @brief scheduler_ calls all registered tasks while they have work.
  /// Once all registered tasks have completed their work, scheduler_ calls the
  /// idle function. The `void Idle()` method provides the logic to wait for a
  /// new program cycle and initiate the next scheduling step. To start new
  /// iteration, user code must calls NotifyGive() periodically.
  void Idle() {
    // Start runtime profiling when give notify and complete runtime here.
    profiler_.runtime_.Stop();

    // After all work is completed, scheduler_ calls the idle implementation
    // (see SetIdleCallback()). Here, it takes a semaphore and waits for the
    // next program cycle.
    new_cycle_ready_sem_.Take(paraos::max_delay);
  }

 private:
  paraos::v2::Thread thread_;
  etl::ischeduler &scheduler_;
  SemaphoreBinary new_cycle_ready_sem_;

  /// @brief Member function object, Need for registered Idle() method in
  /// scheduler_.
  etl::function<ICooperativeScheduling, void> idle_callback;

  struct {
    TimerProfiler period_;
    TimerProfiler runtime_;
  } profiler_;
};

/// @brief Params to pass in CooperativeScheduling{} ctor.
struct CooperativeSchedulingAttr : public ICooperativeSchedulingAttr {};

template <
    std::size_t MAX_TASKS_,
    typename TSchedulerPolicy = etl::scheduler_policy_sequential_single>
class CooperativeScheduling : public ICooperativeScheduling {
 public:
  explicit CooperativeScheduling(const CooperativeSchedulingAttr &attr)
      : ICooperativeScheduling{attr, scheduler_, attr.embedded_timer_} {}

  /// @brief Five rule.
  CooperativeScheduling(CooperativeScheduling &&other) = delete;
  auto operator=(CooperativeScheduling &&other)
      -> CooperativeScheduling & = delete;
  auto operator=(const CooperativeScheduling &other)
      -> CooperativeScheduling & = delete;
  CooperativeScheduling(const CooperativeScheduling &other) = delete;

  ~CooperativeScheduling() override = default;

 private:
  etl::scheduler<TSchedulerPolicy, MAX_TASKS_> scheduler_;
};

}  // namespace  paraos

#endif /* PARAOS_THREAD_COOPERATIVE_SCHEDULING_HPP */
