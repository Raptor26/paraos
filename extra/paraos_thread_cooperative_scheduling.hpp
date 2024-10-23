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
#include "paraos_semaphore.hpp"
#include "paraos_thread.hpp"
#include "paraos_trace.hpp"

namespace paraos {

class ICooperativeScheduling : protected Thread {
  using idle_delegate = etl::delegate<void(void)>;

 public:
  ICooperativeScheduling(
      const std::string name, const std::size_t stack_depth,
      const ThreadPriority priority, etl::ischeduler &scheduler)
      : Thread{name, stack_depth, priority},
        scheduler_{scheduler},
        idle_callback(*this, &ICooperativeScheduling::Idle) {
    // scheduler_ will call all registered task while they has work. Only
    // all registered tasks work complete, scheduler_ call idle function. Here
    // registered idle function which take semaphore and wait new program
    // cycle.
    SetIdleCallback(idle_callback);
  }

  virtual ~ICooperativeScheduling() {}

  void AddTask(etl::task &task) {
    paraos::CriticalSection critical;
    scheduler_.add_task(task);
  }

  void SetIdleCallback(etl::ifunction<void> &callback) {
    paraos::CriticalSection critical;
    scheduler_.set_idle_callback(callback);
  }

  void Run() override {
    try {
      // Method below has internal loop.
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

  bool NotifyGive(bool is_isr = false) {
    return new_cycle_ready_sem_.Give(is_isr);
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
};

template <
    size_t MAX_TASKS_,
    typename TSchedulerPolicy = etl::scheduler_policy_sequential_single>
class CooperativeScheduling : public ICooperativeScheduling {
 public:
  CooperativeScheduling(
      const std::string name, const std::size_t stack_depth,
      const ThreadPriority priority, bool is_need_loop = true)
      : ICooperativeScheduling{name, stack_depth, priority, scheduler_} {
    // Run() method must call in forever loop periodical.
    Thread::SetNeedWhile(is_need_loop);

    // Method below create thread and scheduling it's for execute in RTOS (or
    // windows/unix).
    Thread::Start();
  }

  virtual ~CooperativeScheduling() = default;

 private:
  etl::scheduler<TSchedulerPolicy, MAX_TASKS_> scheduler_;
};

}  // namespace  paraos

#endif /* PARAOS_THREAD_COOPERATIVE_SCHEDULING_HPP */
