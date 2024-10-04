
#include <iostream>
#include <string>
#include <string_view>

#include "paraos_semaphore.hpp"
#include "paraos_timer.hpp"

paraos::SemaphoreBinary sem;
constexpr std::size_t period_ms{1000};

struct UserTimer : public paraos::Timer {
  UserTimer(std::string_view str, std::size_t period_ms)
      : Timer{period_ms, true, true, str}, str_{str} {}

  virtual ~UserTimer() = default;

  void Run() override { std::cout << str_ << std::endl; }

 private:
  std::string_view str_;
};

struct UserTimerWithCnt : public paraos::Timer {
  UserTimerWithCnt(std::string_view str, std::size_t period_ms)
      : Timer{period_ms, true, true, str}, str_{str} {}

  virtual ~UserTimerWithCnt() = default;

  void Run() override {
    std::cout << str_ << " cnt is: " << cnt << std::endl;
    cnt += period_ms;

    if (cnt > runtime_max_ms_) {
      sem.Give();
    }
  }

 private:
  std::string_view str_;
  std::size_t cnt{0};
  static constexpr std::size_t runtime_max_ms_{1000};
};

struct OneShotTimer : public paraos::Timer {
  OneShotTimer(std::string_view str, std::size_t period_ms)
      : Timer{period_ms, true, false, str}, str_{str} {}

  virtual ~OneShotTimer() = default;

  void Run() override { std::cout << str_ << std::endl; }

 private:
  std::string_view str_;
};

UserTimer user_timer{"Global timer", 500u};

int main() {
  OneShotTimer one_shot_timer("One shot timer", 0);
  UserTimer user_timer{"Local timer", 100u};
  UserTimerWithCnt local_timer("Local timer repetition", period_ms);
  auto is_timer_started = local_timer.Start();
  PARAOS_CHECK_ASSERT(is_timer_started);
  PARAOS_ATTR_UNUSED_VAR(is_timer_started);
  usleep(100 * 1000);
  local_timer.Stop();
  std::cout << " timer is stopped " << std::endl;
  local_timer.Start();

  sem.Take();
  return 0;
}
