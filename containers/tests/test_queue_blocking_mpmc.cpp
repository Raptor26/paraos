/// @file test_queue_blocking_spmc_v3.cpp
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

// NOLINTBEGIN(misc-include-cleaner, readability-magic-numbers)
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <iostream>

#include "etl/atomic.h"
#include "paraos_check.h"
#include "paraos_critical.hpp"
#include "paraos_queue_blocking.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_thread.hpp"
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
paraos::QueueBlocking<char, max_queue_size> queue;

paraos::Thread check_test_complete_and_exit{paraos::ThreadAttr{
    "Check test complete", paraos::GetStackMinimumSizeInBytes(),
    paraos::ThreadPriority::kRealTime, nullptr}};
}  // namespace

struct Producer {
  explicit Producer(const paraos::ThreadAttr &attr, std::size_t thread_id)
      : thread_{attr}, thread_id_{thread_id} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<Producer, &Producer::Run>(*this));
  }

  /// @brief Producer thread.
  void Run() {
    char symb{'a'};

    while (true) {
      PrintDebug(" call queue.TryPush()", thread_.GiveName());

      runtime_profiler.Start();
      if (queue.TryPush(symb)) {
        ++push_item_cnt;
        runtime_profiler.Stop();

        PrintDebug(
            " queue.TryPush() success and put "
                << "'" << symb << "'"
                << "" << ". Real delay is "
                << runtime_profiler.LastDurationMs(),
            thread_.GiveName());

        PrintDebug(" exiting ... ", thread_.GiveName());
        ++producers_exit_numb;
        thread_.Finished();
        break;
      }
      PrintDebug(
          " WARN: queue.TryPush() no space, try again "
              << runtime_profiler.LastDurationMs(),
          thread_.GiveName());

      // Yeld processor time for consumers read data from queue.
      paraos::Thread::DelayMs(10);
    }
  }

 private:
  paraos::Thread thread_;

  const std::size_t thread_id_;

  paraos::OsProfiler runtime_profiler;
};

struct Consumer {
  explicit Consumer(const paraos::ThreadAttr &attr, std::size_t thread_id)
      : thread_{attr}, thread_id_{thread_id} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<Consumer, &Consumer::Run>(*this));
  }

  /// @brief Consumer thread.
  void Run() {
    // Small delay for yeld resources for other threads.
    constexpr std::size_t timeout_ms{1};

    PrintDebug(
        " call queue.Pop() with " << timeout_ms << " ms timeout",
        thread_.GiveName());

    runtime_profiler.Start();
    auto read_item = queue.Pop(timeout_ms);
    runtime_profiler.Stop();

    if (read_item) {
      ++pop_item_cnt;
      PrintDebug(" successfully read item from queue", thread_.GiveName());
      PrintDebug(" exiting ... ", thread_.GiveName());
      ++consumers_exit_numb;
      thread_.Finished();
    } else {
      PrintDebug(
          "--ERROR: " << " don't read item from queue with timeout. Try again",
          thread_.GiveName());
    }
  }

 private:
  paraos::Thread thread_;

  const std::size_t thread_id_;

  paraos::OsProfiler runtime_profiler;
};

namespace {
void CheckIfTestSuccessfullyComplete() {
  PARAOS_CHECK_ASSERT(
      push_item_cnt == expected_total_items_in_queue &&
      "Pushed items cnt not equal expected value");

  PARAOS_CHECK_ASSERT(
      push_item_cnt == pop_item_cnt && "Pushed items cnt not equal read");
}

void ExitFromTest() {
  if (((consumers_exit_numb >= consumers_total_numb) &&
       (producers_exit_numb >= producers_total_numb))) {
    check_test_complete_and_exit.Finished();

    constexpr std::size_t delay_ms{0};
    PrintDebug("Ready to exit, delay ms " << delay_ms, "ExitFromTest");
    paraos::Thread::DelayMs(delay_ms);

    CheckIfTestSuccessfullyComplete();

    PrintDebug("Call paraos::Thread::Exit();", "ExitFromTest");

#if defined(PARAOS_LIKE_FREERTOS)
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

  // ---------------------------------------------------------------------------
  // Create consumers
  // ---------------------------------------------------------------------------
  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Consumer 1";
    const static Consumer cons_1{attr, 0};
    consumers_total_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Consumer 2";
    const static Consumer cons_2{attr, 1};
    consumers_total_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Consumer 3";
    const static Consumer cons_3{attr, 2};
    consumers_total_numb += 1;
  }

  // ---------------------------------------------------------------------------
  // Create producers
  // ---------------------------------------------------------------------------
  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Prod 1";
    const static Producer prod_1{attr, 0};
    producers_total_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Prod 2";
    const static Producer prod_2{attr, 1};
    producers_total_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Prod 3";
    const static Producer prod_3{attr, 2};
    producers_total_numb += 1;
  }

  expected_total_items_in_queue =
      producers_total_numb * one_producer_expected_push_items_numb;

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  return 0;
}
// NOLINTEND(misc-include-cleaner, readability-magic-numbers)
