/// @file test_queue_blocking_mpmc.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
// NOLINTBEGIN(misc-include-cleaner, readability-magic-numbers)
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "etl/atomic.h"
#include "paraos_check.h"
#include "paraos_critical.hpp"
#include "paraos_jthread.hpp"
#include "paraos_queue_blocking.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_sleep.hpp"
#include "paraos_thread_common.hpp"
#include "paraos_utils.hpp"

#define PrintDebug(__message__, __object_name__)               \
  {                                                            \
    const paraos::CriticalSection macro_critical;              \
                                                               \
    const std::time_t result = std::time(nullptr);             \
                                                               \
    std::cout << "Time: '" << result << " " << __object_name__ \
              << "': " << __message__ << "\n";                 \
  }

namespace {

constexpr std::size_t max_queue_size{2};
constexpr std::size_t one_producer_expected_push_items_numb{1};

etl::atomic<std::size_t> producers_total_numb{0};
etl::atomic<std::size_t> consumers_total_numb{0};
etl::atomic<std::size_t> producers_exit_numb{0};
etl::atomic<std::size_t> consumers_exit_numb{0};
etl::atomic<std::size_t> expected_total_items_in_queue{0};

std::atomic_size_t push_item_cnt{0};
std::atomic_size_t pop_item_cnt{0};

std::mutex g_done_mtx;
std::condition_variable g_done_cv;
bool g_scheduler_ended{false};

paraos::QueueBlocking<char, max_queue_size> queue;

void NotifySchedulerEnded() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  const std::scoped_lock lock{g_done_mtx};
  g_scheduler_ended = true;
  g_done_cv.notify_one();
}

void WaitForSchedulerEnded() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  std::unique_lock lock{g_done_mtx};
  g_done_cv.wait(lock, []() -> bool { return g_scheduler_ended; });
}

struct Producer {  // NOLINT(hicpp-special-member-functions)
  explicit Producer(std::string_view name) : name_{name} {}

  ~Producer() { PrintDebug("~Dtor:", name_); }

  /// @brief Producer thread.
  void operator()(const paraos::stop_token& token) {
    const char symb{'a'};

    paraos::OsProfiler runtime_profiler;

    while (!token.stop_requested()) {
      PrintDebug(" call queue.TryPush()", name_);

      runtime_profiler.Start();
      if (queue.TryPush(symb)) {
        ++push_item_cnt;
        runtime_profiler.Stop();

        PrintDebug(
            " queue.TryPush() success and put "
                << "'" << symb << "'"
                << "" << ". Real delay is "
                << runtime_profiler.LastDurationMs(),
            name_);

        PrintDebug(" exiting ... ", name_);
        ++producers_exit_numb;
        return;
      }
      PrintDebug(
          " WARN: queue.TryPush() no space, try again "
              << runtime_profiler.LastDurationMs(),
          name_);

      // Yield processor time for consumers read data from queue.
      paraos::sleep_for(std::chrono::milliseconds(10));
    }
  }

 private:
  std::string_view name_;
};

struct Consumer {  // NOLINT(hicpp-special-member-functions)
  explicit Consumer(std::string_view name) : name_{name} {}

  ~Consumer() { PrintDebug("~Dtor:", name_); }

  /// @brief Consumer thread.
  void operator()(const paraos::stop_token& token) {
    // Small delay for yeld resources for other threads.
    constexpr std::size_t timeout_ms{2000};

    paraos::OsProfiler runtime_profiler;

    while (!token.stop_requested()) {
      PrintDebug(
          " call queue.Pop() with " << timeout_ms << " ms timeout", name_);

      runtime_profiler.Start();
      auto read_item = queue.Pop(timeout_ms);
      runtime_profiler.Stop();

      if (read_item) {
        ++pop_item_cnt;
        PrintDebug(" successfully read item from queue", name_);
        PrintDebug(" exiting ... ", name_);
        ++consumers_exit_numb;
        return;
      }
      PrintDebug(
          "--ERROR: " << " don't read item from queue with timeout. Try again",
          name_);
    }
  }

 private:
  std::string_view name_;
};

void CheckIfTestSuccessfullyComplete(  // NOLINT(llvm-prefer-static-over-anonymous-namespace): using static triggers misc-use-anonymous-namespace; keep internal linkage via anonymous namespace.
) {
  PARAOS_CHECK_ASSERT(
      push_item_cnt == expected_total_items_in_queue &&
      "Pushed items cnt not equal expected value");

  PARAOS_CHECK_ASSERT(
      push_item_cnt == pop_item_cnt && "Pushed items cnt not equal read");
}

void IdleHook() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  WaitForSchedulerEnded();
  CheckIfTestSuccessfullyComplete();
  (void)paraos::jthread::end_scheduler();
}

}  // namespace

auto main() -> int {
  producers_total_numb = 3;
  consumers_total_numb = 3;
  expected_total_items_in_queue =
      producers_total_numb * one_producer_expected_push_items_numb;

  // ---------------------------------------------------------------------------
  // Create consumers and producers inside an inner scope so that jthread
  // destructors join before final assertions.
  // ---------------------------------------------------------------------------
  {
    std::vector<paraos::jthread> threads;

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Consumer 0";
      threads.emplace_back(attr, Consumer{"--Consumer 0"});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Consumer 1";
      threads.emplace_back(attr, Consumer{"--Consumer 1"});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Consumer 2";
      threads.emplace_back(attr, Consumer{"--Consumer 2"});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Prod 0";
      threads.emplace_back(attr, Producer{"--Prod 0"});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Prod 1";
      threads.emplace_back(attr, Producer{"--Prod 1"});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Prod 2";
      threads.emplace_back(attr, Producer{"--Prod 2"});
    }

    const paraos::jthread stopper(
        [](const paraos::stop_token& /*token*/) -> void {
          while (pop_item_cnt.load() < expected_total_items_in_queue.load()) {
            paraos::sleep_for(std::chrono::milliseconds{10});
          }
          NotifySchedulerEnded();
        });

#if PARAOS_LIKE_FREERTOS
    paraos::freertos_idle_fnc_ptr = IdleHook;
#endif

    paraos::jthread::start_scheduler();

    // При использовании freeRTOS, мы никогда не попадем в строку ниже т.к. все
    // управление блокируется в paraos::jthread::start_scheduler();
    WaitForSchedulerEnded();
  }

  // В методе ниже проверяется что все данные считаны и вызывается
  // (void)paraos::jthread::end_scheduler(); Весь функционал завершения работы
  // потоков инкапсулирован в одном методе чтобы его можно было использовать в
  // paraos::freertos_idle_fnc_ptr при тестах freeRTOS. Это связано с тем, что
  // метод ниже никогда не будет вызван при использовании freeRTOS (из-за
  // перехвата управления при вызове paraos::jthread::start_scheduler(), поэтому
  // передается указатель на IdleHook, который периодически вызывается на
  // freERTOS)
  IdleHook();

  return 0;
}
// NOLINTEND(misc-include-cleaner, readability-magic-numbers)
