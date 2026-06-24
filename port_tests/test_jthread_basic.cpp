/// @file test_jthread_basic.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>

#include "paraos_jthread.hpp"
#include "paraos_sleep.hpp"
#include "paraos_utils.hpp"

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables,
// *-readability-identifier-naming)

namespace {
std::atomic<std::size_t> counter{0};
std::atomic<bool> g_self_destruct_ok{false};
constexpr std::size_t kExpectedCounter{2};

std::mutex g_done_mtx;
std::condition_variable g_done_cv;
bool g_scheduler_ended{false};

void my_func(int a, float f) {
  counter.fetch_add(1);
  (void)a;
  (void)f;
}

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

void IdleHook() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  WaitForSchedulerEnded();

  if (counter.load() == kExpectedCounter) {
    std::cout << "OK\n";
  } else {
    std::cout << "FAIL: counter=" << counter.load() << "\n";
    std::exit(EXIT_FAILURE);
  }

  (void)paraos::jthread::end_scheduler();
}
}  // namespace

auto main() -> int {
  {
    std::size_t size{0};
    paraos::jthread thread([&size](const paraos::stop_token& token) {
      counter.fetch_add(1);
      while (!token.stop_requested()) {
        ++size;
        paraos::sleep_for(std::chrono::milliseconds{10});
      }
    });

    paraos::jthread thread2([](const paraos::stop_token& token) {
      my_func(42, 3.14f);
      while (!token.stop_requested()) {
        paraos::sleep_for(std::chrono::milliseconds{10});
      }
    });

    paraos::jthread self_destruct_thread(
        [&self_destruct_thread](const paraos::stop_token& token) {
          // Calling join() from inside the owned thread must return
          // immediately without blocking. With the self-join guard this
          // must not deadlock.
          self_destruct_thread.join();
          g_self_destruct_ok.store(true);
          while (!token.stop_requested()) {
            paraos::sleep_for(std::chrono::milliseconds{10});
          }
        });

    const paraos::jthread stopper(
        [](const paraos::stop_token& /*token*/) -> void {
          while (counter.load() < kExpectedCounter ||
                 !g_self_destruct_ok.load()) {
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
