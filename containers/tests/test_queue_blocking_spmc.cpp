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

constexpr std::size_t max_queue_size{2};

constexpr std::size_t total_items_to_be_pushed{5};

namespace {
paraos::Thread check_test_complete_and_exit{paraos::ThreadAttr{
    "Check test complete", paraos::GetStackMinimumSizeInBytes(),
    paraos::ThreadPriority::kRealTime, nullptr}};

etl::atomic<std::size_t> producer_thread_numb{0};

etl::atomic<std::size_t> consumer_thread_numb{0};

etl::atomic<std::size_t> producer_thread_exit_cnt{0};

etl::atomic<std::size_t> consumer_thread_exit_cnt{0};

std::atomic_size_t push_item_cnt{0};

std::atomic_size_t pop_item_cnt{0};

paraos::QueueBlocking<char, max_queue_size> queue;
}  // namespace

struct Producer {
  explicit Producer(const paraos::ThreadAttr &attr) : thread_{attr} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<Producer, &Producer::Run>(*this));
  }

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
        ++symb;
        if (push_item_cnt >= total_items_to_be_pushed) {
          PrintDebug(" exiting ... ", thread_.GiveName());
          producer_thread_exit_cnt++;
          thread_.Finished();
          break;
        }
      } else {
        PrintDebug(
            " WARN: queue.TryPush() no space, try again "
                << runtime_profiler.LastDurationMs(),
            thread_.GiveName());
      }
      // Yeld processor time for consumers read data from queue.
      paraos::Thread::DelayMs(1);
    }
  }

 private:
  paraos::Thread thread_;

  paraos::OsProfiler runtime_profiler;
};

struct Consumer {
  explicit Consumer(const paraos::ThreadAttr &attr) : thread_{attr} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<Consumer, &Consumer::Run>(*this));
  }

  /// @brief Consumer thread.
  void Run() {
    constexpr std::size_t timeout_ms{2000};

    while (true) {
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
        consumer_thread_exit_cnt++;
        thread_.Finished();
        break;
      }
      PrintDebug(
          "--ERROR: don't read item from queue with timeout. Try again",
          thread_.GiveName());
    }
  }

 private:
  paraos::Thread thread_;

  paraos::OsProfiler runtime_profiler;
};

namespace {
void CheckIfTestSuccessfullyComplete() {
  const paraos::CriticalSection critical;

  PARAOS_CHECK_ASSERT(
      push_item_cnt == total_items_to_be_pushed &&
      "Pushed items cnt not equal expected value");

  PARAOS_CHECK_ASSERT(
      push_item_cnt == pop_item_cnt && "Pushed items cnt not equal read");
}

void ExitFromTest() {
  if (((consumer_thread_exit_cnt >= consumer_thread_numb) &&
       (producer_thread_exit_cnt >= producer_thread_numb))) {
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
  // Create producers
  // ---------------------------------------------------------------------------
  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 0";
    const static Producer prod_0{attr};
    producer_thread_numb += 1;
  }
  // ---------------------------------------------------------------------------

  // ---------------------------------------------------------------------------
  // Create consumers
  // ---------------------------------------------------------------------------
  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 0";
    const static Consumer cons_0{attr};
    consumer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 1";
    const static Consumer cons_1{attr};
    consumer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 2";
    const static Consumer cons_2{attr};
    consumer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 3";
    const static Consumer cons_3{attr};
    consumer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 4";
    const static Consumer cons_4{attr};
    consumer_thread_numb += 1;
  }

  // ---------------------------------------------------------------------------

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  return 0;
}
// NOLINTEND(misc-include-cleaner, readability-magic-numbers)
