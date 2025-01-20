/// @file test_paraos_cooperative_scheduling.cpp
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

#include <cstdint>
#include <cstdlib>
#include <iostream>

// Useless check here because static analyzer cant see usage of some headers,
// but they're actually used in tis file.
// NOLINTBEGIN(misc-include-cleaner)
#include "etl/function.h"
#include "etl/scheduler.h"
#include "etl/task.h"
#include "paraos_runtime_profiler.hpp"
#include "paraos_thread.hpp"
#include "paraos_thread_cooperative_scheduling.hpp"
#include "paraos_utils.hpp"
// NOLINTEND(misc-include-cleaner)

namespace {
bool is_test_complete{false};

// Task 1 set highest priority in set. It will run first.
constexpr etl::task_priority_t task1_priority{10};
constexpr etl::task_priority_t task2_priority{9};
constexpr etl::task_priority_t task3_priority{8};

constexpr size_t max_tasks_number{10};
}  // namespace

class Task1 : public etl::task {
 public:
  //*************************************
  Task1() : task(task1_priority), work(3) {}

  //*************************************
  [[nodiscard]] auto task_request_work() const -> uint32_t override {
    return work;  // How much work do we still have to do? This could be a
                  // message queue length.
  }

  //*************************************
  void task_process_work() override {
    std::cout << "Task1 : Process work : " << work << "\n";
    --work;
  }

 private:
  uint32_t work{0};
};

class Task2 : public etl::task {
 public:
  //*************************************
  Task2() : task(task2_priority), work(3) {}

  //*************************************
  [[nodiscard]] auto task_request_work() const -> uint32_t override {
    return work;  // How much work do we still have to do? This could be a
                  // message queue length.
  }

  //*************************************
  void task_process_work() override {
    std::cout << "Task2 : Process work : " << work << "\n";
    --work;
  }

 private:
  uint32_t work{0};
};

class Task3 : public etl::task {
 public:
  //*************************************
  Task3() : task(task3_priority), work(1) {}

  //*************************************
  [[nodiscard]] auto task_request_work() const -> uint32_t override {
    return work;  // How much work do we still have to do? This could be a
                  // message queue length.
  }

  //*************************************
  void task_process_work() override {
    std::cout << "Task3 : Process work : " << work << "\n";
    --work;
  }

 private:
  uint32_t work{0};
};

class Idle {
 public:
  //*************************************
  explicit Idle(etl::ischeduler& scheduler_) : scheduler(scheduler_) {}

  //*************************************
  void IdleCallback() {
    std::cout << "Idle callback" << "\n";
    scheduler.exit_scheduler();
    std::cout << "Exiting the scheduler" << "\n";

    // Call exit(EXIT_SUCCESS) in ExitAfterTestComplete() for force break system
    // process (in freertos port only).
    is_test_complete = true;
  }

 private:
  etl::ischeduler& scheduler;
};

// -----------------------------------------------------------------------------
// Global definitions for variables need for freertos port. When called
// exit(EXIT_SUCCESS), global object call their destructions (for local object
// nothing calls). It's help to reduced memory check warnings.
// -----------------------------------------------------------------------------

namespace {
paraos::CooperativeScheduling<
    max_tasks_number, etl::scheduler_policy_highest_priority>
    cooperative_scheduler{paraos::CooperativeSchedulingAttr{
        "Cooperative", paraos::GetStackMinimumSizeInBytes(),
        paraos::ThreadPriority::kNormal, paraos::embedded_timer_empty, false,
        true}};

Idle idle_handle(cooperative_scheduler.GetScheduler());

etl::function_mv<Idle, &Idle::IdleCallback> idle_callback(idle_handle);

Task1 task1;
Task2 task2;
Task3 task3;

/// FreeRTOS can't stop scheduler. In this case we must manually call
/// exit(EXIT_SUCCESS) after test complete.
#if defined(FREERTOS)
void ExitAfterTestComplete() {
  if (is_test_complete) {
    exit(EXIT_SUCCESS);
  }
}
#endif
}  // namespace

auto main() -> int {
#if defined(FREERTOS)
  // ExitAfterTestComplete will be called by scheduler in idle task after no
  // user task ready for execute.
  paraos::freertos_idle_fnc_ptr = ExitAfterTestComplete;
#endif

  // When calling AddTask(), scheduler compare priority each task and sorted
  // tasks references in private vector with tasks priority respect.
  cooperative_scheduler.AddTask(task3);
  cooperative_scheduler.AddTask(task1);
  cooperative_scheduler.AddTask(task2);

  cooperative_scheduler.SetIdleCallback(idle_callback);

  cooperative_scheduler.NotifyGive();

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  return EXIT_SUCCESS;
}