/// @file test_paraos_timer.cpp
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

#include <cstddef>
#include <iostream>
#include <string_view>

#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_semaphore.hpp"
#include "paraos_thread.hpp"
#include "paraos_timer.hpp"

namespace {
paraos::SemaphoreBinary sem;
}  // namespace

constexpr std::size_t period_ms{10};
constexpr std::size_t global_timer_period_ms{500};
constexpr std::size_t local_timer_period_ms{100};

constexpr std::size_t thread_waiting_delay_ms{100};

struct UserTimer : public paraos::Timer {
  UserTimer(std::string_view str, std::size_t period_ms)
      : Timer{period_ms, true}, str_{str} {}

  ~UserTimer() override = default;

  void Run() override { std::cout << str_ << "\n"; }

  /// @brief Five rule.
  UserTimer(UserTimer &&other) = delete;
  auto operator=(UserTimer &&other) -> UserTimer & = delete;
  auto operator=(const UserTimer &other) -> UserTimer & = delete;
  UserTimer(const UserTimer &other) = delete;

 private:
  std::string_view str_;
};

struct UserTimerWithCnt : public paraos::Timer {
  UserTimerWithCnt(std::string_view str, std::size_t period_ms)
      : Timer{period_ms, false}, str_{str} {}

  ~UserTimerWithCnt() override = default;

  void Run() override {
    std::cout << str_ << " cnt is: " << cnt << "\n";
    cnt += period_ms;

    if (cnt > runtime_max_ms_) {
      sem.Give();
    }
  }

  /// @brief Five rule.
  UserTimerWithCnt(UserTimerWithCnt &&other) = delete;
  auto operator=(UserTimerWithCnt &&other) -> UserTimerWithCnt & = delete;
  auto operator=(const UserTimerWithCnt &other) -> UserTimerWithCnt & = delete;
  UserTimerWithCnt(const UserTimerWithCnt &other) = delete;

 private:
  std::string_view str_;
  std::size_t cnt{0};
  static constexpr std::size_t runtime_max_ms_{1000};
};

namespace {
UserTimer user_timer{"Global timer", global_timer_period_ms};
}  // namespace

auto main() -> int {
  const UserTimer user_timer{"Local timer", local_timer_period_ms};
  UserTimerWithCnt local_timer("Local timer repetition", period_ms);
  auto is_timer_started = local_timer.Start();
  PARAOS_CHECK_ASSERT(is_timer_started);
  PARAOS_ATTR_UNUSED_VAR(is_timer_started);
  paraos::Thread::DelayMs(thread_waiting_delay_ms);
  local_timer.Stop();
  std::cout << " timer is stopped " << "\n";
  local_timer.Start();

  sem.Take();
  return 0;
}
