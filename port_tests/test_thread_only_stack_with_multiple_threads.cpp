/// @file test_thread_only_stack_with_multiple_threads.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "paraos_base.hpp"
#include "paraos_critical.hpp"
#include "paraos_jthread.hpp"
#include "paraos_semaphore_std.hpp"
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

void CheckIfTestSuccessfullyComplete() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  constexpr auto k_expected_deleted_objects{EXPECTED_THREADS * 2};
  if (deleted_objects_cnt.load() != k_expected_deleted_objects) {
    std::cout << "FAIL: deleted_objects_cnt=" << deleted_objects_cnt.load()
              << "\n";
    std::exit(EXIT_FAILURE);
  }

  std::cout << "OK\n";
}

void IdleHook() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  WaitForSchedulerEnded();
  CheckIfTestSuccessfullyComplete();
  (void)paraos::jthread::end_scheduler();
}

struct MyThreadGroup {
  explicit MyThreadGroup(std::string_view name)
      : thread_one_{
            paraos::ThreadAttr{
                std::string{name}, paraos::GetStackMinimumSizeInBytes(),
                paraos::ThreadPriority::kNormal},
            [this](const paraos::stop_token& /*token*/) {
              PrintDebug("ProcessingOne() calls", name_);
              is_thread_one_complete_work_.release();
            }},
        thread_two_{
            paraos::ThreadAttr{
                std::string{name} + " two",
                paraos::GetStackMinimumSizeInBytes(),
                paraos::ThreadPriority::kNormal},
            [this](const paraos::stop_token& /*token*/) {
              PrintDebug("ProcessingTwo() wait sems", name_);
              is_thread_one_complete_work_.acquire();
              is_dynamic_thread_complete_work_.acquire();
              PrintDebug("ProcessingTwo() calls Finished(this)", name_);
              DeletedObjectsCnt();
            }},
        thread_dynamic_{
            paraos::ThreadAttr{
                std::string{name} + " Dynamic thread",
                paraos::GetStackMinimumSizeInBytes(),
                paraos::ThreadPriority::kNormal},
            [this](const paraos::stop_token& /*token*/) {
              PrintDebug("ProcessingDynamic is finish", name_);
              is_dynamic_thread_complete_work_.release();
            }},
        name_{name} {}

  ~MyThreadGroup() {
    PrintDebug("~MyThreadGroup", name_);
    DeletedObjectsCnt();
  }

  paraos::jthread thread_one_;
  paraos::jthread thread_two_;
  paraos::jthread thread_dynamic_;
  paraos::binary_semaphore is_thread_one_complete_work_{0};
  paraos::binary_semaphore is_dynamic_thread_complete_work_{0};
  std::string name_;
};
}  // namespace

auto main() -> int {
  {
    std::vector<std::unique_ptr<MyThreadGroup>> groups;

    try {
      for (std::size_t i = 0; i < EXPECTED_THREADS; ++i) {
        const std::string name{"My thread for stack " + std::to_string(i)};
        groups.emplace_back(std::make_unique<MyThreadGroup>(name));
      }
    } catch (const paraos::thread_exception& e) {
      PrintDebug(e.what(), "main()");
    } catch (const std::exception& e) {
      PrintDebug(e.what(), "main()");
    }

    const paraos::jthread stopper(
        [&groups](const paraos::stop_token& /*token*/) -> void {
          while (deleted_objects_cnt.load() < EXPECTED_THREADS) {
            paraos::sleep_for(std::chrono::milliseconds{10});
          }

          // Explicitly destroy the groups so their destructors run while the
          // scheduler is still active, replicating the original test's
          // "self-delete on completion" behavior in an RAII-safe way.
          groups.clear();

          constexpr auto k_expected_deleted_objects{EXPECTED_THREADS * 2};
          while (deleted_objects_cnt.load() < k_expected_deleted_objects) {
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
