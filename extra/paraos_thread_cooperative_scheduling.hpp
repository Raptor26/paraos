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
#include "paraos_critical.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_thread.hpp"
#include "paraos_trace.hpp"

namespace paraos {

/// @brief Интерфейс для управления расписанием потоков.
class ICooperativeScheduling : protected Thread {
  using idle_delegate = etl::delegate<void(void)>;

 public:
  ICooperativeScheduling(
      const std::string name, const std::size_t stack_depth,
      const ThreadPriority priority, etl::ischeduler &scheduler,
      const IEmbeddedTimer &embedded_timer)
      : Thread{name, stack_depth, priority},
        scheduler_{scheduler},
        idle_callback(*this, &ICooperativeScheduling::Idle) {
    // scheduler_ will call all registered task while they has work. Only
    // all registered tasks work complete, scheduler_ call idle function. Here
    // registered idle function which take semaphore and wait new program
    // cycle.
    SetIdleCallback(idle_callback);

    // Connect embedded timers for each profiler, using in a
    // ICooperativeScheduling.
    runtime.period_.SetEmbeddedTimer(embedded_timer);
  }

  virtual ~ICooperativeScheduling() { Exit(); }

  /// @brief Stop any tasks executions in cooperative scheduler.
  ///
  /// @note Useful in unit tests when need exit from cooperative scheduler.
  void Exit() {
    const paraos::CriticalSection critical;

    if (!is_exit_calls_) {
      SetNeedWhile(false);
      scheduler_.exit_scheduler();

      // Force give notify for leave while cycle inside cooperative scheduler.
      // Need because if Exit() calls cooperative scheduler may wait notify
      // forever in Idle().
      NotifyGive();

      is_exit_calls_ = true;
    }
  }

  /// @brief Added task in list for execute when Run() calls. 'task' position in
  /// list depend by task priority (task priority set in task ctor). That's
  /// mean, task with higher priority will call first on each scheduler
  /// iteration.
  ///
  /// @param[in] task: task for put in private list. That's mean task will
  /// scheduling for execute when Run() calls in paraos thread context.
  virtual bool AddTask(etl::task &task) {
    bool is_task_add{false};
    try {
      paraos::CriticalSection critical;
      scheduler_.add_task(task);
      is_task_add = true;
    } catch (const etl::scheduler_too_many_tasks_exception &e) {
      paraosTRACE_MESSAGE(
          e.file_name() << "; --line: " << e.line_number()
                        << "; --what: " << e.what());
    }

    return is_task_add;
  }

  /// @brief After all tasks work complete in one iteration, scheduler call idle
  /// task. User can set custom idle function for calling by scheduler when no
  /// anymore work in one iteration.
  ///
  /// @param[in] callback: User function, which calls after all works complete.
  void SetIdleCallback(etl::ifunction<void> &callback) {
    paraos::CriticalSection critical;
    scheduler_.set_idle_callback(callback);
  }

  void Run() override {
    try {
      // Method below has internal forever loop (for break internal forever loop
      // need call scheduler_.exit_scheduler()).
      scheduler_.start();
    } catch (etl::scheduler_no_tasks_exception &e) {
      paraosTRACE_MESSAGE(
          e.file_name() << "; --line: " << e.line_number()
                        << "; --what: " << e.what());

      // Run() method call in loop. When no tasks for execute,
      // scheduler_.start() throw exception. After Run() catch exception,
      // scheduler_.start() will call immediately in forever loop (Run() execute
      // in external forever loop). In this case all processor time will be
      // wasted. So, DelayMs() yeld processor time for other threads. After any
      // task was registered (when user code call AddTask()), scheduler_.start()
      // start execute in internal loop, which blocking void Idle() method by
      // taking semaphore.
      Thread::DelayMs(1000);
    } catch (etl::exception &e) {
      paraosTRACE_MESSAGE(
          e.file_name() << "; --line: " << e.line_number()
                        << "; --what: " << e.what());
    }
  }

  /// @brief Cooperative scheduler run periodical. That's mean user code must
  /// give notify periodical.
  ///
  /// @param[in] is_isr: Set true if calls from interrupt.
  ///
  /// @return Return true if notify successfully given.
  bool NotifyGive(const bool is_isr = false) {
    auto is_notify_given = new_cycle_ready_sem_.Give(is_isr);

    // Sequence below need for calculate period between calls NotifyGive();
    runtime.period_.Stop();
    runtime.period_.Start();

    return is_notify_given;
  }

  auto &GetScheduler() { return scheduler_; }

 private:
  /// @brief scheduler_ will call all registered task while they has work. Only
  /// all registered tasks work complete, scheduler_ call idle function. 'void
  /// Idle()' provide code for wait new program cycle and start new scheduling
  /// step.
  void Idle() {
    // After all works complete, scheduler_ call our idle implementation (see
    // SetIdleCallback()). Here take semaphore and wait next program
    // cycle.
    new_cycle_ready_sem_.Take(paraos::max_delay);
  }

 private:
  etl::ischeduler &scheduler_;
  SemaphoreBinary new_cycle_ready_sem_;

  /// @brief Member function object, Need for registered Idle() method in
  /// scheduler_.
  etl::function<ICooperativeScheduling, void> idle_callback;

  bool is_exit_calls_{false};

  struct {
    TimerProfiler period_;
  } runtime;
};

struct CooperativeSchedulingAttr {
  std::string name{"Cooperative scheduler"};
  std::size_t stack_depth = GetStackMinimumSizeInBytes();
  ThreadPriority priority = ThreadPriority::kAboveNormal;

  const IEmbeddedTimer &embedded_timer_ = embedded_timer_empty;

  bool is_need_loop{true};
  bool is_need_start{true};
};

template <
    size_t MAX_TASKS_,
    typename TSchedulerPolicy = etl::scheduler_policy_sequential_single>
class CooperativeScheduling
    : public etl::scheduler<TSchedulerPolicy, MAX_TASKS_>,
      public ICooperativeScheduling {
 public:
  CooperativeScheduling(const CooperativeSchedulingAttr &attr)
      : ICooperativeScheduling{
            attr.name, attr.stack_depth, attr.priority, *this,
            attr.embedded_timer_} {
    // Run() method must call in forever loop periodical.
    Thread::SetNeedWhile(attr.is_need_loop);

    // Set 'is_need_start = false' useful for unit tests.
    if (attr.is_need_start) {
      // Method below create thread and scheduling it's for execute in RTOS (or
      // windows/unix).
      Thread::Start();
    }
  }

  virtual ~CooperativeScheduling() = default;
};

}  // namespace  paraos

#endif /* PARAOS_THREAD_COOPERATIVE_SCHEDULING_HPP */
