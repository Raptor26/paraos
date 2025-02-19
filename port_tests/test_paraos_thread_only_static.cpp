/// @file test-paraos_thread_only_static.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2025 Stilsoft
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

#include <stdio.h>

#if defined(FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif

#include <unistd.h>

#include <atomic>
#include <iostream>
#include <memory>
#include <string>

#include "paraos_critical.hpp"
#include "paraos_thread_v2.hpp"
#include "paraos_timer.hpp"

#define PrintDebug(__message__, __object_name__)                    \
  {                                                                 \
    const paraos::CriticalSection macro_critical;                   \
    std::cout << "DM: '" << __object_name__ << "': " << __message__ \
              << std::endl;                                         \
  }

std::atomic<std::size_t> deleted_objects_cnt;

void DeletedObjectsCnt() {
  ++deleted_objects_cnt;

  PrintDebug(
      "Deleted objects cnt is " << deleted_objects_cnt, "DeletedObjectsCnt");
}

std::atomic<std::size_t> cnt{0};

constexpr std::size_t expected_threads{3};

inline void DefaultDelegate() { paraos::v2::Thread::DelayMs(100); }
constexpr paraos::v2::thread_delegate_type thread_default_delegate =
    etl::delegate<void()>::create<DefaultDelegate>();

paraos::v2::Thread check_test_complete_and_exit{paraos::v2::ThreadAttr{
    "Check test complete", paraos::GetStackMinimumSizeInBytes(),
    paraos::v2::ThreadPriority::kRealTime, DeletedObjectsCnt,
    thread_default_delegate}};

void ExitFromTest() {
  if (cnt >= expected_threads) {
    check_test_complete_and_exit.Finished();
    constexpr std::size_t delay_ms{0};
    PrintDebug("Ready to exit, delay ms " << delay_ms, "ExitFromTest");

    check_test_complete_and_exit.Finished();

    paraos::v2::Thread::DelayMs(delay_ms);

    PrintDebug("Call paraos::v2::Thread::Exit();", "ExitFromTest");
    paraos::v2::Thread::Exit();
  }

  PrintDebug("Yeld resources", "ExitFromTest");
  paraos::v2::Thread::DelayMs(10);
}

class MyThreadStatic {
 public:
  MyThreadStatic(const paraos::v2::ThreadAttr& attr) : thread_{attr} {
    thread_.RegisterDelegate(
        paraos::v2::thread_delegate_type::create<
            MyThreadStatic, &MyThreadStatic::Processing>(*this));
  }

  virtual ~MyThreadStatic() = default;

  void Processing() {
    PrintDebug("Calling Processing()", thread_.GiveName());

    PrintDebug("Calling thread_.Finished()", thread_.GiveName());
    thread_.Finished();
    ++cnt;
  }

 private:
  paraos::v2::Thread thread_;
};

auto main() -> int {
  {
    auto delegate = etl::delegate<void()>::create<ExitFromTest>();
    check_test_complete_and_exit.RegisterDelegate(delegate);
  }

  {
    paraos::v2::ThreadAttr attr;
    attr.thread_name = "My thread static one";
    attr.dtor_callback = DeletedObjectsCnt;

    static MyThreadStatic static_thread{attr};
  }

  {
    paraos::v2::ThreadAttr attr;
    attr.thread_name = "My thread static two";
    attr.dtor_callback = DeletedObjectsCnt;
    static MyThreadStatic static_thread{attr};
  }

  {
    paraos::v2::ThreadAttr attr;
    attr.thread_name = "My thread static three";
    attr.dtor_callback = DeletedObjectsCnt;
    static MyThreadStatic static_thread{attr};
  }

  paraos::v2::Thread::StartScheduler();
  paraos::v2::Thread::DeleteAll();

  return 0;
}
