/// @file example_thread_check_timeout.cpp
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

#include "iostream"
#include "paraos_runtime_profiler.hpp"
#include "paraos_thread.hpp"

using namespace paraos;

std::atomic_bool is_test_complete{false};

struct TestTimeout : public paraos::Thread {
  TestTimeout(const std::string name = "default thread name")
      : paraos::Thread{name, 3072ull, paraos::ThreadPriority::kNormal, true} {
    Start();
  }

  void Run() override {
    OsProfiler profiler;

    std::size_t delay_ms{1000};

    constexpr std::size_t delay_one_iteration{200};

    profiler.Start();
    auto current_time = Thread::GetCurrentTime();
    while (true) {
      if (Thread::CheckTimeout(current_time, delay_ms)) {
        break;
      }

      std::cout << "Sleep inside cycle " << delay_one_iteration << " ms"
                << std::endl;
      Thread::SleepMs(delay_one_iteration);
    }

    profiler.Stop();

    std::cout << "--Cycle total time is " << profiler.LastDurationMs() << " ms"
              << std::endl;

    is_test_complete = true;
  }
};

void ExitAfterTestComplete() {
  if (is_test_complete) {
    std::cout << "Exiting program..." << std::endl;
    exit(EXIT_SUCCESS);
  }
}

int main() {
#if defined(FREERTOS)
  // ExitAfterTestComplete will be called by scheduler in idle task after no
  // user task ready for execute.
  paraos::freertos_idle_fnc_ptr = ExitAfterTestComplete;
#endif

  TestTimeout test_thread("Check timeout");

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  std::cout << "Exiting program..." << std::endl;
  return EXIT_SUCCESS;
}