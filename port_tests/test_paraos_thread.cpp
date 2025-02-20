/// @file test_thread_join.cpp
/// @author Mickle Isaev <mrraptor26@gmail.com>
/// @author VyhodcevEgor <vyhodcev@internet.ru>
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
#include <cstdlib>
#include <iostream>
#include <vector>

#include "paraos_critical.hpp"
#include "paraos_thread_common.hpp"
#include "paraos_thread_v2.hpp"

namespace {
std::vector<const paraos::v2::Thread*> thread_ptr;

std::size_t cnt{0};

std::size_t threads_count{0};

std::size_t threads_exit_count{0};

paraos::v2::Thread check_test_complete_and_exit{paraos::v2::ThreadAttr{
    "Check test complete", paraos::GetStackMinimumSizeInBytes(),
    paraos::v2::ThreadPriority::kLowest, nullptr}};

#if defined(FREERTOS)
/// @brief Hack for unit test. When used freeRTOS, we can't return from main
/// regular way after  paraos::Thread::StartScheduler() called. For finish
/// test program, we need call exit(). But in this case, sanitizer print
/// warning with `Potential Memory Leak`. For reduced sanitizer warnings,
/// forced call dtor for complete threads. Container below needed to force
/// Dtor call for threads that have complete their execution in
/// ExitAfterTestComplete().
void ExitAfterTestComplete() {
  static bool threads_deleted_flag{false};

  if (cnt == 3) {
    std::cout << "Deleting all threads..." << "\n";

    for (const auto* thread : thread_ptr) {
      // Force call Dtor for registered threads befor call exit(EXIT_SUCCESS);
      thread->~Thread();
    }

    threads_deleted_flag = true;
    cnt++;
  }

  if (cnt == 4 && threads_deleted_flag) {
    std::cout << "Exiting program..." << "\n";
    exit(EXIT_SUCCESS);
  }
}
#endif

void ExitFromTest() {
  if (threads_exit_count >= threads_count) {
    check_test_complete_and_exit.Finished();
    paraos::v2::Thread::Exit();
  }

  std::cout << "ExitFromTest Yeld resources" << "\n";
  paraos::v2::Thread::DelayMs(10);
}
}  // namespace

struct TestMessage {
  explicit TestMessage(const paraos::v2::ThreadAttr& attr) : thread_{attr} {
    thread_.RegisterDelegate(paraos::v2::thread_delegate_type::create<
                             TestMessage, &TestMessage::Run>(*this));
  }

  void Run() {
    const paraos::CriticalSection critical;
    std::cout << thread_.GiveName() << " RTOS thread Cnt is " << cnt << "\n";
    ++cnt;
    if (cnt > 3) {
      thread_.Finished();
      threads_exit_count++;
    }
  }

 private:
  paraos::v2::Thread thread_;
};

auto main() -> int {
  {
    static auto delegate = etl::delegate<void()>::create<ExitFromTest>();
    check_test_complete_and_exit.RegisterDelegate(delegate);
  }

#if defined(FREERTOS)
#include "paraos_utils.hpp"

  // ExitAfterTestComplete will be called by scheduler in idle task after no
  // user task ready for execute.
  paraos::freertos_idle_fnc_ptr = ExitAfterTestComplete;
#endif

  {
    paraos::v2::ThreadAttr attr{};
    attr.thread_name = "Thread 1";
    const static TestMessage print1{attr};
    threads_count += 1;
  }

  {
    paraos::v2::ThreadAttr attr{};
    attr.thread_name = "Thread 2";
    const static TestMessage print2{attr};
    threads_count += 1;
  }

  {
    paraos::v2::ThreadAttr attr{};
    attr.thread_name = "Thread 3";
    const static TestMessage print3{attr};
    threads_count += 1;
  }

  paraos::v2::Thread::StartScheduler();
  paraos::v2::Thread::DeleteAll();

  return EXIT_SUCCESS;
}
// NOLINTEND(misc-include-cleaner, readability-magic-numbers)
