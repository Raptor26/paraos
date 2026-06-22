/// @file example_thread_check_timeout.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>

#include "paraos_critical.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_thread.hpp"
#include "paraos_thread_common.hpp"
#include "paraos_time.hpp"

constexpr std::size_t thread_default_stack_depth{
    paraos::GetStackMinimumSizeInBytes() * 3};

#define PrintDebug(__message__, __object_name__)                             \
  {                                                                          \
    const paraos::CriticalSection macro_critical;                            \
    std::cout << "DM: '" << __object_name__ << "': " << __message__ << "\n"; \
  }

namespace {
std::atomic_bool is_test_complete{false};

paraos::Thread check_test_complete_and_exit{paraos::ThreadAttr{
    "Check test complete", paraos::GetStackMinimumSizeInBytes(),
    paraos::ThreadPriority::kLowest, nullptr}};

// String copy here is needed because of the delayed thread initialization -
// address of it's name could be invalid later.
// NOLINTBEGIN(performance-unnecessary-value-param)
struct TestTimeout {
  explicit TestTimeout(const paraos::ThreadAttr& attr) : thread_{attr} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<TestTimeout, &TestTimeout::Run>(
            *this));
  }

  void Run() {
    paraos::OsProfiler profiler;

    constexpr paraos::delay_type expected_delay_ms{1000};
    paraos::delay_type delay_ms{expected_delay_ms};

    constexpr paraos::delay_type delay_one_iteration{200};

    profiler.Start();
    // Useless check here, because clang-tidy somehow can't see the
    // GetCurrentTime() definition inside paraos::Thread.
    // NOLINTBEGIN(misc-include-cleaner)
    auto current_time = paraos::GetCurrentTime();
    // NOLINTEND(misc-include-cleaner)
    while (true) {
      if (paraos::CheckTimeout(current_time, delay_ms)) {
        break;
      }

      // Wait sem, nobody give them, we check total delay (delay_ms).
      sem_.Take(delay_one_iteration);

      std::cout << "Sleep inside cycle " << delay_one_iteration << " ms."
                << " New delay_ms is " << delay_ms << "\n";
    }

    profiler.Stop();

    std::cout << "--Cycle total time is " << profiler.LastDurationMs() << " ms."
              << " Expected delay is " << expected_delay_ms << " ms." << "\n";

    is_test_complete = true;
  }

 private:
  paraos::SemaphoreBinary sem_;
  paraos::Thread thread_;
};
// NOLINTEND(performance-unnecessary-value-param)

void ExitFromTest(  // NOLINT(llvm-prefer-static-over-anonymous-namespace): using static triggers misc-use-anonymous-namespace; keep internal linkage via anonymous namespace.
) {
  if (is_test_complete) {
    check_test_complete_and_exit.Finished();

    constexpr paraos::delay_type delay_ms{0};
    PrintDebug("Ready to exit, delay ms " << delay_ms, "ExitFromTest");
    paraos::Thread::DelayMs(delay_ms);

    PrintDebug("Call paraos::Thread::Exit();", "ExitFromTest");
    paraos::Thread::Exit();
  }
  // NOLINTNEXTLINE readability-magic-numbers
  paraos::Thread::DelayMs(10);
}
}  // namespace

auto main() -> int {
  {
    static auto delegate = etl::delegate<void()>::create<ExitFromTest>();
    check_test_complete_and_exit.RegisterDelegate(delegate);
  }

  paraos::ThreadAttr attr{};
  attr.thread_name = "Check timeout";
  attr.stack_depth = thread_default_stack_depth;
  const TestTimeout test_thread(attr);

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();
}