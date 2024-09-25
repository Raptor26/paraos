/// @file example_timer.cpp
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

#include "iostream"
#include "paraos_thread.hpp"
#include "paraos_timer.hpp"

constexpr std::size_t period_ms{100};

struct UserTimer : public paraos::Timer {
  UserTimer(std::string_view str, std::size_t period_ms)
      : Timer{period_ms, true}, str_{str} {}

  virtual ~UserTimer() = default;

  void Run() override { std::cout << str_ << std::endl; }

 private:
  std::string_view str_;
};

struct UserTimerWithCnt : public paraos::Timer {
  UserTimerWithCnt(std::string_view str, std::size_t period_ms)
      : Timer{period_ms, false}, str_{str} {}

  virtual ~UserTimerWithCnt() = default;

  void Run() override {
    std::cout << str_ << " cnt is: " << cnt << std::endl;
    cnt += period_ms;

    if (cnt > runtime_max_ms_) {
      exit(0);
    }
  }

 private:
  std::string_view str_;
  std::size_t cnt{0};
  static constexpr std::size_t runtime_max_ms_{1000};
};

int main() {
  UserTimerWithCnt use_timer{"local timer", 100};
  use_timer.Start();

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  return 0;
}