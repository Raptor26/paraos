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

#include <atomic>
#include <cstddef>
#include <string>

#include "paraos_check.h"
#include "paraos_critical.hpp"
#include "paraos_queue_blocking.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_thread.hpp"
#include "paraos_trace.hpp"
#include "paraos_utils.hpp"

constexpr std::size_t max_queue_size{2};

constexpr std::size_t one_producer_expected_push_items_numb{3};

constexpr std::size_t threads_default_stack_depth{1024};
namespace {
std::atomic_size_t push_item_cnt{0};
std::atomic_size_t pop_item_cnt{0};

paraos::QueueBlocking<char, max_queue_size> queue;
}  // namespace

// String copy here is needed because of the delayed thread initialization -
// address of it's name could be invalid later.
// NOLINTBEGIN(performance-unnecessary-value-param)
struct Producer : public paraos::Thread {
  explicit Producer(
      const std::string name = "Producer",
      std::size_t stack_depth = threads_default_stack_depth,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kIdle)
      : paraos::Thread{name, stack_depth, priority} {
    paraos::Thread::SetNeedWhile(true);
    Start();
  }

  void Run() override {
    char symb{'a'};

    while (true) {
      paraosTRACE_MESSAGE(Name() << " call queue.TryPush()");
      runtime_profiler.Start();
      if (queue.TryPush(symb)) {
        ++push_item_cnt;
        runtime_profiler.Stop();
        paraosTRACE_MESSAGE(
            Name() << " queue.TryPush() success and put " << "'" << symb << "'"
                   << "" << ". Real delay is "
                   << runtime_profiler.LastDurationMs());
        ++symb;
        break;
      }
      paraosTRACE_MESSAGE(
          Name() << " WARN: queue.TryPush() no space, try again "
                 << runtime_profiler.LastDurationMs());
      // Yeld processor time for consumers read data from queue.
      DelayMs(1);
    }

    paraos::Thread::SetNeedWhile(false);
  }

 private:
  paraos::OsProfiler runtime_profiler;
};

struct Consumer : public paraos::Thread {
  explicit Consumer(
      const std::string name = "Consumer",
      std::size_t stack_depth = threads_default_stack_depth,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kIdle)
      : paraos::Thread{name, stack_depth, priority} {
    paraos::Thread::SetNeedWhile(true);
    Start();
  }

  /// @brief Consumer thread.
  void Run() override {
    constexpr std::size_t timeout_ms{2000};

    for (std::size_t i = 0; i < one_producer_expected_push_items_numb; ++i) {
      while (true) {
        paraosTRACE_MESSAGE(
            Name() << " call queue.Pop() with " << timeout_ms << " ms timeout");
        runtime_profiler.Start();
        auto read_item = queue.Pop(timeout_ms);
        runtime_profiler.Stop();

        if (read_item) {
          ++pop_item_cnt;
          paraosTRACE_MESSAGE(Name() << " successfully read item from queue");
          break;
        }
        const paraos::CriticalSection critical;
        paraosTRACE_MESSAGE(
            "--ERROR: "
            << Name() << " don't read item from queue with timeout. Try again");
      }
    }

    paraos::Thread::SetNeedWhile(false);

    paraosTRACE_MESSAGE(Name() << " exiting ..");
  }

 private:
  paraos::OsProfiler runtime_profiler;
};
// NOLINTEND(performance-unnecessary-value-param)

namespace {
void CheckIfTestSuccessfullyComplete() {
  const paraos::CriticalSection critical;

  PARAOS_CHECK_ASSERT(
      push_item_cnt == one_producer_expected_push_items_numb &&
      "Pushed items cnt not equal expected value");

  PARAOS_CHECK_ASSERT(
      push_item_cnt == pop_item_cnt && "Pushed items cnt not equal read");
}

/// FreeRTOS can't stop scheduler. In this case we must manually call
/// exit(EXIT_SUCCESS) after test complete.
#if defined(FREERTOS)
#include <cstdlib>

void ExitAfterTestComplete() {
  const paraos::CriticalSection critical;
  if ((push_item_cnt == pop_item_cnt) &&
      (push_item_cnt == one_producer_expected_push_items_numb)) {
    CheckIfTestSuccessfullyComplete();
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

  const Consumer consumer1{
      "--Consumer 1", paraos::GetStackMinimumSizeInBytes(),
      paraos::ThreadPriority::kHighest};

  const Producer producer1{
      "Producer 1", paraos::GetStackMinimumSizeInBytes(),
      paraos::ThreadPriority::kNormal};
  const Producer producer2{
      "Producer 2", paraos::GetStackMinimumSizeInBytes(),
      paraos::ThreadPriority::kNormal};
  const Producer producer3{
      "Producer 3", paraos::GetStackMinimumSizeInBytes(),
      paraos::ThreadPriority::kNormal};

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  return 0;
}