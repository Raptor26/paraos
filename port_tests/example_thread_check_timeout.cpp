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
#include <cstddef>
#include <cstdlib>
#include <string>

#include "iostream"
#include "paraos_runtime_profiler.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_thread.hpp"

constexpr std::size_t thread_default_stack_depth{3072};

namespace {
std::atomic_bool is_test_complete{false};

#if defined(FREERTOS)
void ExitAfterTestComplete() {
  if (is_test_complete) {
    std::cout << "Exiting program..." << "\n";
    exit(EXIT_SUCCESS);
  }
}
#endif
}  // namespace

// String copy here is needed because of the delayed thread initialization -
// address of it's name could be invalid later.
// NOLINTBEGIN(performance-unnecessary-value-param)
struct TestTimeout : public paraos::Thread {
  explicit TestTimeout(const std::string name = "default thread name")
      : paraos::Thread{
            name, thread_default_stack_depth, paraos::ThreadPriority::kNormal,
            true} {
    Start();
  }

  void Run() override {
    paraos::OsProfiler profiler;

    constexpr std::size_t expected_delay_ms{1000};
    std::size_t delay_ms{expected_delay_ms};

    constexpr std::size_t delay_one_iteration{200};

    profiler.Start();
    // Useless check here, because clang-tidy somehow can't see the
    // GetCurrentTime() definition inside paraos::Thread.
    // NOLINTBEGIN(misc-include-cleaner)
    auto current_time = paraos::Thread::GetCurrentTime();
    // NOLINTEND(misc-include-cleaner)
    while (true) {
      if (Thread::CheckTimeout(current_time, delay_ms)) {
        break;
      }

      // Wait sem, nobody give them, we check total delay (delay_ms).
      sem_.Take(delay_one_iteration);

      std::cout << "Sleep inside cycle " << delay_one_iteration << " ms."
                << " New delay_ms is " << delay_ms << "\n";
    }

    profiler.Stop();

    std::cout << "--Cycle total time is " << profiler.LastDurationMs() << " ms."
              << " Expected delay is " << expected_delay_ms << " ms."
              << "\n";

    is_test_complete = true;
  }

 private:
  paraos::SemaphoreBinary sem_;
};
// NOLINTEND(performance-unnecessary-value-param)

auto main() -> int {
#if defined(FREERTOS)
#include "paraos_utils.hpp"

  // ExitAfterTestComplete will be called by scheduler in idle task after no
  // user task ready for execute.
  paraos::freertos_idle_fnc_ptr = ExitAfterTestComplete;
#endif

  const TestTimeout test_thread("Check timeout");

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  std::cout << "Exiting program..." << "\n";
  return EXIT_SUCCESS;
}