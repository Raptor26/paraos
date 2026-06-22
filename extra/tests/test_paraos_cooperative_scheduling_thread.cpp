/// @file test_paraos_cooperative_scheduling_thread.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <cstdint>
#include <cstdlib>
#include <iostream>

// Useless check here because static analyzer cant see usage of some headers,
// but they're actually used in tis file.
// NOLINTBEGIN(misc-include-cleaner, readability-magic-numbers)
#include "etl/atomic.h"
#include "etl/function.h"
#include "etl/scheduler.h"
#include "etl/task.h"
#include "paraos_runtime_profiler.hpp"
#include "paraos_thread.hpp"
#include "paraos_thread_cooperative_scheduling.hpp"
#include "paraos_utils.hpp"

#define PrintDebug(__message__, __object_name__)                             \
  {                                                                          \
    const paraos::CriticalSection macro_critical;                            \
    std::cout << "DM: '" << __object_name__ << "': " << __message__ << "\n"; \
  }

namespace {
etl::atomic_bool is_test_complete{false};

paraos::Thread check_test_complete_and_exit{paraos::ThreadAttr{
    "Check test complete", paraos::GetStackMinimumSizeInBytes(),
    paraos::ThreadPriority::kRealTime, nullptr}};

// Task 1 set highest priority in set. It will run first.
constexpr etl::task_priority_t task1_priority{10};
constexpr etl::task_priority_t task2_priority{9};
constexpr etl::task_priority_t task3_priority{8};

constexpr size_t max_tasks_number{10};

class Task1 : public etl::task {
 public:
  //*************************************
  Task1() : task(task1_priority), work{3} {}

  //*************************************
  [[nodiscard]] auto task_request_work() const -> uint32_t override {
    return work;  // How much work do we still have to do? This could be a
                  // message queue length.
  }

  //*************************************
  void task_process_work() override {
    PrintDebug("Task1 : Process work : " << work, "");
    --work;
  }

 private:
  uint32_t work{0};
};

class Task2 : public etl::task {
 public:
  //*************************************
  Task2() : task(task2_priority), work{3} {}

  //*************************************
  [[nodiscard]] auto task_request_work() const -> uint32_t override {
    return work;  // How much work do we still have to do? This could be a
                  // message queue length.
  }

  //*************************************
  void task_process_work() override {
    PrintDebug("Task2 : Process work : " << work, "");
    --work;
  }

 private:
  uint32_t work{0};
};

class Task3 : public etl::task {
 public:
  //*************************************
  Task3() : task(task3_priority), work{1} {}

  //*************************************
  [[nodiscard]] auto task_request_work() const -> uint32_t override {
    return work;  // How much work do we still have to do? This could be a
                  // message queue length.
  }

  //*************************************
  void task_process_work() override {
    PrintDebug("Task3 : Process work : " << work, "");
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

    // Call exit(EXIT_SUCCESS) in ExitAfterTestComplete() for force break system
    // process (in freertos port only).
    is_test_complete = true;

    scheduler.exit_scheduler();
  }

 private:
  etl::ischeduler& scheduler;
};

// -----------------------------------------------------------------------------
// Global definitions for variables need for freertos port. When called
// exit(EXIT_SUCCESS), global object call their destructions (for local object
// nothing calls). It's help to reduced memory check warnings.
// -----------------------------------------------------------------------------

paraos::CooperativeScheduling<
    max_tasks_number, etl::scheduler_policy_highest_priority>
    cooperative_scheduler{paraos::CooperativeSchedulingAttr{
        {{"Cooperative scheduler", paraos::GetStackMinimumSizeInBytes(),
          paraos::ThreadPriority::kRealTime, nullptr}}}};

Idle idle_handle(cooperative_scheduler.GetScheduler());

etl::function_mv<Idle, &Idle::IdleCallback> idle_callback(idle_handle);

Task1 task1;
Task2 task2;
Task3 task3;

void ExitFromTest(  // NOLINT(llvm-prefer-static-over-anonymous-namespace): using static triggers misc-use-anonymous-namespace; keep internal linkage via anonymous namespace.
) {
  if (is_test_complete) {
    check_test_complete_and_exit.Finished();

    // Exit from cooperative scheduler.
    cooperative_scheduler.Finish(false);

    constexpr std::size_t delay_ms{0};
    PrintDebug("Ready to exit, delay ms " << delay_ms, "ExitFromTest");
    paraos::Thread::DelayMs(delay_ms);

    PrintDebug("Call paraos::Thread::Exit();", "ExitFromTest");
#ifdef PARAOS_LIKE_FREERTOS
    // Forces program exit to reduce execution time. Needed to terminate tests
    // early, especially when running multiple tests. In other case, program
    // will exit in 1 second later.
    std::_Exit(EXIT_SUCCESS);
#else
    paraos::Thread::Exit();
#endif
  }

  PrintDebug("Yeld resources", "ExitFromTest");
  paraos::Thread::DelayMs(10);
}
}  // namespace

auto main() -> int {
  {
    static auto delegate = etl::delegate<void()>::create<ExitFromTest>();
    check_test_complete_and_exit.RegisterDelegate(delegate);
  }

  // When calling AddTask(), scheduler compare priority each task and sorted
  // tasks references in private vector with tasks priority respect.
  cooperative_scheduler.AddTask(task3);
  cooperative_scheduler.AddTask(task1);
  cooperative_scheduler.AddTask(task2);

  // Set custom idle callback to complete test.
  cooperative_scheduler.SetIdleCallback(idle_callback);

  paraos::Thread::StartScheduler();

  paraos::Thread::DeleteAll();

  return EXIT_SUCCESS;
}
// NOLINTEND(misc-include-cleaner, readability-magic-numbers)
