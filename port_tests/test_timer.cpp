/// @file test_timer.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// @brief Standalone safety test for paraos::timer.

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <mutex>

#include "paraos_jthread.hpp"
#include "paraos_sleep.hpp"
#include "paraos_timer.hpp"
#include "paraos_utils.hpp"

namespace {

constexpr std::size_t k_period_ms{100};
constexpr std::size_t k_self_stop_period_ms{200};
constexpr std::size_t k_short_period_ms{50};
constexpr std::chrono::milliseconds k_wait_step_ms{10};
constexpr std::chrono::milliseconds k_max_wait_ms{1000};

std::atomic<bool> g_basic_done{false};
std::atomic<bool> g_self_stop_done{false};
std::atomic<bool> g_destructor_done{false};
std::atomic<bool> g_cycles_done{false};

std::mutex g_done_mtx;
std::condition_variable g_done_cv;
bool g_scheduler_ended{false};

class test_timer_app : public paraos::timer {
 public:
  test_timer_app() : paraos::timer(k_period_ms, false, true, "test_timer") {}

  void run() override {}
};

class self_stopping_timer : public paraos::timer {
 public:
  self_stopping_timer()
      : paraos::timer(
            k_self_stop_period_ms, false, true, "self_stopping_timer") {}

  void run() override {
    stopped_in_run_.store(true, std::memory_order_release);
    this->stop();
  }

  [[nodiscard]] auto stopped_in_run() const -> const std::atomic<bool>& {
    return stopped_in_run_;
  }

 private:
  std::atomic<bool> stopped_in_run_{false};
};

auto wait_for(const std::atomic<bool>& flag) -> bool {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  const auto start = std::chrono::steady_clock::now();
  while (!flag.load(std::memory_order_acquire)) {
    paraos::sleep_for(k_wait_step_ms);
    if (std::chrono::steady_clock::now() - start > k_max_wait_ms) {
      return false;
    }
  }
  return true;
}

void notify_scheduler_ended() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  {
    const std::scoped_lock lock{g_done_mtx};
    g_scheduler_ended = true;
  }
  g_done_cv.notify_one();
}

void wait_for_scheduler_ended() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  std::unique_lock lock{g_done_mtx};
  g_done_cv.wait(lock, []() -> bool { return g_scheduler_ended; });
}

void run_tests() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  // Basic start/stop.
  {
    test_timer_app timer;
    timer.start();
    timer.stop();
  }
  g_basic_done.store(true, std::memory_order_release);

  // Self-stop from inside run() must not deadlock.
  {
    self_stopping_timer timer;
    timer.start();
    if (wait_for(timer.stopped_in_run())) {
      timer.stop();
      g_self_stop_done.store(true, std::memory_order_release);
    }
  }

  // Destructor must stop and join an active timer without hanging.
  {
    test_timer_app timer;
    timer.start();
  }
  g_destructor_done.store(true, std::memory_order_release);

  // Repeated start/change_period/stop cycles must not leak threads or overflow
  // the wakeup semaphore.
  {
    test_timer_app timer;
    for (int i = 0; i < 3; ++i) {
      timer.start();
      timer.change_period(k_short_period_ms);
      timer.stop();
    }
  }
  g_cycles_done.store(true, std::memory_order_release);

  notify_scheduler_ended();
}

void idle_hook() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  wait_for_scheduler_ended();

  const bool is_ok = g_basic_done.load(std::memory_order_acquire) &&
                     g_self_stop_done.load(std::memory_order_acquire) &&
                     g_destructor_done.load(std::memory_order_acquire) &&
                     g_cycles_done.load(std::memory_order_acquire);

  if (!is_ok) {
    std::exit(EXIT_FAILURE);
  }

  (void)paraos::jthread::end_scheduler();
}

}  // namespace

auto main() -> int {
#if PARAOS_LIKE_FREERTOS
  paraos::freertos_idle_fnc_ptr = idle_hook;
#endif

  const paraos::jthread test_runner(
      [](const paraos::stop_token& /*token*/) -> void { run_tests(); });

  paraos::jthread::start_scheduler();

  // On FreeRTOS control never reaches here; idle_hook validates and shuts down
  // the scheduler from the idle task context.
  wait_for_scheduler_ended();
  idle_hook();

  return 0;
}
