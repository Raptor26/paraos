/// @file test_thread_only_stack.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "paraos_base.hpp"
#include "paraos_critical.hpp"
#include "paraos_jthread.hpp"
#include "paraos_sleep.hpp"
#include "paraos_thread_common.hpp"
#include "paraos_thread_exceptions.hpp"
#include "paraos_utils.hpp"

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables,
// *-readability-identifier-naming)

#define PrintDebug(__message__, __object_name__)                             \
  {                                                                          \
    const paraos::CriticalSection macro_critical;                            \
    std::cout << "DM: '" << __object_name__ << "': " << __message__ << "\n"; \
  }

namespace {
std::atomic<std::size_t> cnt{0};

constexpr std::size_t EXPECTED_THREADS{3};

std::atomic<std::size_t> deleted_objects_cnt;

void DeletedObjectsCnt() {
  ++deleted_objects_cnt;

  PrintDebug(
      "Deleted objects cnt is " << deleted_objects_cnt, "DeletedObjectsCnt");
}

std::mutex g_done_mtx;
std::condition_variable g_done_cv;
bool g_scheduler_ended{false};

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

void IdleHook() {
  WaitForSchedulerEnded();

  if (deleted_objects_cnt.load() == EXPECTED_THREADS) {
    std::cout << "OK\n";
  } else {
    std::cout << "FAIL: deleted_objects_cnt=" << deleted_objects_cnt.load()
              << "\n";
    std::exit(EXIT_FAILURE);
  }

  (void)paraos::jthread::end_scheduler();
}
}  // namespace

auto main() -> int {
  {
    std::vector<paraos::jthread> threads;

    try {
      for (std::size_t i = 0; i < EXPECTED_THREADS; ++i) {
        const std::string name{"My thread dynamic " + std::to_string(i)};
        const paraos::ThreadAttr attr{
            name, paraos::GetStackMinimumSizeInBytes(),
            paraos::ThreadPriority::kNormal};

        threads.emplace_back(attr, [i](const paraos::stop_token& /*token*/) {
          PrintDebug(
              "Calls Processing()", "My thread dynamic " + std::to_string(i));
          ++cnt;
          DeletedObjectsCnt();
        });
      }
    } catch (const paraos::thread_exception& e) {
      PrintDebug(e.what(), "main()");
    } catch (const std::exception& e) {
      PrintDebug(e.what(), "main()");
    }

    const paraos::jthread stopper(
        [](const paraos::stop_token& /*token*/) -> void {
          while (deleted_objects_cnt.load() < EXPECTED_THREADS) {
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

  // В методе ниже проверяется результат и вызывается
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

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables,
// *-readability-identifier-naming)
