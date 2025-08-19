/// @file test_oneshot_executor_thread.cpp
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
///
/// @copyright (c) 2025 Stilsoft
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

#include <iostream>

#include "etl/atomic.h"
#include "paraos_oneshot_executor.hpp"

// clang-format off
// NOLINTBEGIN (*-err58-cpp, *-macro-parentheses, *-global-variables, *-exception-escape, *-member-functions, *-identifier-naming)
// clang-format on

#define PrintDebug(__message__, __object_name__)                             \
  {                                                                          \
    const paraos::CriticalSection macro_critical;                            \
    std::cout << "DM: '" << __object_name__ << "': " << __message__ << "\n"; \
  }

namespace {

paraos::Thread check_test_complete_and_exit{paraos::ThreadAttr{
    "Check test complete", paraos::GetStackMinimumSizeInBytes(),
    paraos::ThreadPriority::kRealTime, nullptr}};

etl::atomic_bool is_test_complete{false};

constexpr uint_least8_t max_delegates_in_queue{4};
using OneShotExecutorTest = paraos::OneShotExecutor<max_delegates_in_queue>;
OneShotExecutorTest *oneshot_executor_ptr;

class Producer {
 public:
  Producer() = default;

  void Produce() {
    cnt_++;
    std::cout << "Producer called " << cnt_ << " times." << "\n";

    if (cnt_ == 2) {
      is_test_complete = true;
    }
  }

  ~Producer() = default;

 private:
  size_t cnt_{0};
};

class Worker {
 public:
  Worker() = default;

  void Work() {
    cnt_++;
    std::cout << "Worker called " << cnt_ << " times." << "\n";
  }

  ~Worker() = default;

 private:
  size_t cnt_{0};
};

void ExitFromTest() {
  if (is_test_complete) {
    check_test_complete_and_exit.Finished();

    PARAOS_CHECK_ASSERT(oneshot_executor_ptr);
    oneshot_executor_ptr->Finish();

    constexpr std::size_t delay_ms{0};
    PrintDebug("Ready to exit, delay ms " << delay_ms, "ExitFromTest");
    paraos::Thread::DelayMs(delay_ms);

    PrintDebug("Call paraos::Thread::Exit();", "ExitFromTest");
#if defined(PARAOS_LIKE_FREERTOS)
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

  {
    const paraos::OneShotExecutorAttributes attr{
        {{"OneShotExecutor thread", paraos::GetStackMinimumSizeInBytes(),
          paraos::ThreadPriority::kRealTime, nullptr}}};

    static OneShotExecutorTest oneshot_executor{attr};
    oneshot_executor_ptr = &oneshot_executor;
  }

  static Producer producer{};

  static auto producer_delegate =
      paraos::executor_delegate_type::create<Producer, &Producer::Produce>(
          producer);

  static Worker worker{};

  static auto worker_delegate =
      paraos::executor_delegate_type::create<Worker, &Worker::Work>(worker);

  oneshot_executor_ptr->EnqueueDelegate(producer_delegate);
  oneshot_executor_ptr->EnqueueDelegate<Producer, &Producer::Produce>(producer);

  oneshot_executor_ptr->EnqueueDelegate(worker_delegate);

  oneshot_executor_ptr->EnqueueDelegate(producer_delegate);

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  oneshot_executor_ptr = nullptr;

  return 0;
}
// clang-format off
// NOLINTEND (*-err58-cpp, *-macro-parentheses, *-global-variables, *-exception-escape, *-member-functions, *-identifier-naming)
// clang-format on
