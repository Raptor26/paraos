/// @file example_timer.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
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

constexpr paraos::delay_type period_ms_default{10};
constexpr paraos::delay_type global_timer_period_ms{500};
constexpr paraos::delay_type local_timer_period_ms{100};
constexpr paraos::delay_type thread_waiting_delay_ms{100};

struct UserTimer : public paraos::Timer {
  UserTimer(std::string_view str, paraos::delay_type period_ms)
      : Timer{period_ms, true}, str_{str} {}

  ~UserTimer() override = default;

  void Run() override { std::cout << str_ << "\n"; }

  /// @brief Five rule.
  UserTimer(UserTimer&& other) = delete;
  auto operator=(UserTimer&& other) -> UserTimer& = delete;
  auto operator=(const UserTimer& other) -> UserTimer& = delete;
  UserTimer(const UserTimer& other) = delete;

 private:
  std::string_view str_;
};

struct UserTimerWithCnt : public paraos::Timer {
  UserTimerWithCnt(std::string_view str, paraos::delay_type period_ms)
      : Timer{period_ms, false}, str_{str} {}

  ~UserTimerWithCnt() override = default;

  void Run() override {
    std::cout << str_ << " cnt is: " << cnt << "\n";
    cnt += period_ms_default;

    if (cnt > runtime_max_ms_) {
      sem.Give();
    }
  }

  /// @brief Five rule.
  UserTimerWithCnt(UserTimerWithCnt&& other) = delete;
  auto operator=(UserTimerWithCnt&& other) -> UserTimerWithCnt& = delete;
  auto operator=(const UserTimerWithCnt& other) -> UserTimerWithCnt& = delete;
  UserTimerWithCnt(const UserTimerWithCnt& other) = delete;

 private:
  std::string_view str_;
  std::size_t cnt{0};
  static constexpr std::size_t runtime_max_ms_{1000};
};

UserTimer user_timer{"Global timer", global_timer_period_ms};
}  // namespace

auto main() -> int {
  const UserTimer user_timer_local{"Local timer", local_timer_period_ms};
  UserTimerWithCnt local_timer("Local timer repetition", period_ms_default);
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
