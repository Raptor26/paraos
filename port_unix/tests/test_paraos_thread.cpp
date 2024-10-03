/// @file test_thread.cpp
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

#include <iostream>

#include "paraos_critical.hpp"
#include "paraos_thread.hpp"

struct PrintTestMessage : public paraos::Thread {
  PrintTestMessage(const std::string name = "default thread name")
      : paraos::Thread{name, 1024, paraos::ThreadPriority::kBelowNormal} {
    Start();
  }
  void Run() override {
    const paraos::CriticalSection critical;
    std::cout << Name() << " Cnt is " << cnt_ << std::endl;
    ++cnt_;
  }

 private:
  std::size_t cnt_{0};
};

int main() {
  PrintTestMessage print1{"Thread 1"};
  PrintTestMessage print2{"Thread 2"};
  PrintTestMessage print3{"Thread 3"};
  PrintTestMessage print4{"Thread 4"};
  PrintTestMessage print5{"Thread 5"};
  PrintTestMessage print6{"Thread 6"};

  PrintTestMessage print11{"Thread 11"};
  PrintTestMessage print22{"Thread 22"};
  PrintTestMessage print33{"Thread 33"};
  PrintTestMessage print44{"Thread 44"};
  PrintTestMessage print55{"Thread 55"};
  PrintTestMessage print66{"Thread 66"};

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();
  return 0;
}