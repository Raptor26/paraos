/// @file test_oneshot_executor_thread.cpp
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <mutex>

#include "etl/delegate.h"
#include "paraos_oneshot_executor.hpp"
#include "paraos_sleep.hpp"

// clang-format off
// NOLINTBEGIN (*-err58-cpp, *-macro-parentheses, *-global-variables, *-exception-escape, *-member-functions, *-identifier-naming)
// clang-format on

namespace {

constexpr uint_least8_t max_delegates_in_queue{4};
using OneShotExecutorTest = paraos::OneShotExecutor<max_delegates_in_queue>;

OneShotExecutorTest* g_executor_ptr{nullptr};

std::atomic<std::size_t> producer_call_cnt{0};
std::atomic<std::size_t> worker_call_cnt{0};
std::atomic<bool> is_test_complete{false};

std::mutex g_done_mtx;
std::condition_variable g_done_cv;
bool g_scheduler_ended{false};

class Producer {
 public:
  Producer() = default;

  void Produce() {
    ++producer_call_cnt;
    std::cout << "Producer called " << producer_call_cnt.load() << " times."
              << std::endl;

    if (producer_call_cnt.load() == 2) {
      is_test_complete.store(true, std::memory_order_release);
    }
  }

  ~Producer() = default;
};

class Worker {
 public:
  Worker() = default;

  void Work() {
    ++worker_call_cnt;
    std::cout << "Worker called " << worker_call_cnt.load() << " times."
              << std::endl;
  }

  ~Worker() = default;
};

void NotifySchedulerEnded() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  {
    const std::scoped_lock lock{g_done_mtx};
    g_scheduler_ended = true;
  }
  g_done_cv.notify_one();
}

void WaitForSchedulerEnded() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  std::unique_lock lock{g_done_mtx};
  g_done_cv.wait(lock, []() -> bool { return g_scheduler_ended; });
}

void IdleHook() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  WaitForSchedulerEnded();

  PARAOS_CHECK_ASSERT(producer_call_cnt == 3);
  PARAOS_CHECK_ASSERT(worker_call_cnt == 1);

  constexpr bool is_isr{false};
  PARAOS_CHECK_ASSERT(g_executor_ptr);
  g_executor_ptr->Finish(is_isr);

  (void)paraos::jthread::end_scheduler();
}

}  // namespace

auto main() -> int {
  {
    paraos::OneShotExecutorAttributes attr{
        {{"OneShotExecutor thread", paraos::GetStackMinimumSizeInBytes(),
          paraos::ThreadPriority::kRealTime}}};

    OneShotExecutorTest oneshot_executor{attr};
    g_executor_ptr = &oneshot_executor;

    static Producer producer{};
    static Worker worker{};

    static auto producer_delegate =
        paraos::executor_delegate_type::create<Producer, &Producer::Produce>(
            producer);

    static auto worker_delegate =
        paraos::executor_delegate_type::create<Worker, &Worker::Work>(worker);

    oneshot_executor.EnqueueDelegate(producer_delegate);
    oneshot_executor.EnqueueDelegate<Producer, &Producer::Produce>(producer);
    oneshot_executor.EnqueueDelegate(worker_delegate);
    oneshot_executor.EnqueueDelegate(producer_delegate);

    const paraos::jthread stopper(
        [](const paraos::stop_token& /*token*/) -> void {
          while (!is_test_complete.load(std::memory_order_acquire)) {
            paraos::sleep_for(std::chrono::milliseconds{10});
          }

          NotifySchedulerEnded();
        });
    (void)stopper;

#if PARAOS_LIKE_FREERTOS
    paraos::freertos_idle_fnc_ptr = IdleHook;
#endif

    paraos::jthread::start_scheduler();

    WaitForSchedulerEnded();

    IdleHook();
  }

  return 0;
}
// clang-format off
// NOLINTEND (*-err58-cpp, *-macro-parentheses, *-global-variables, *-exception-escape, *-member-functions, *-identifier-naming)
// clang-format on
