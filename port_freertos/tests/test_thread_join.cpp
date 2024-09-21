/// @file test_thread_join.cpp
/// @author VyhodcevEgor <vyhodcev@internet.ru>
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

#include <cstdlib>
#include <iostream>
#include <vector>

#include "paraos_critical.hpp"
#include "paraos_thread.hpp"

static std::size_t cnt{0};

/// @brief Hack for unit test. When used freeRTOS, we can't return from main
/// regular way after  paraos::Thread::StartScheduler() called. For finish test
/// program, we need call exit(). But in this case, sanitizer print warning with
/// `Potential Memory Leak`. For reduced sanitizer warnings, forced call dtor
/// for complete threads. Container below needed to force Dtor call for threads
/// that have complete their execution in ExitAfterTestComplete().
std::vector<paraos::Thread*> thread_ptr;

static bool threads_deleted_flag = false;
void ExitAfterTestComplete() {
  if (cnt == 3) {
    std::cout << "Deleting all threads..." << std::endl;

    for (auto thread : thread_ptr) {
      // Force call Dtor for registered threads befor call exit(EXIT_SUCCESS);
      thread->~Thread();
    }

    threads_deleted_flag = true;
    cnt++;
  }

  if (cnt == 4 && threads_deleted_flag == true) {
    std::cout << "Exiting program..." << std::endl;
    exit(EXIT_SUCCESS);
  }
}

struct TestMessage : public paraos::Thread {
  TestMessage(const std::string name = "default thread name")
      : paraos::Thread{name, 3072ull, paraos::ThreadPriority::kNormal, true} {
    Start();
  }
  void Run() override {
    const paraos::CriticalSection critical;
    std::cout << Name() << " RTOS thread Cnt is " << cnt << std::endl;
    ++cnt;
  }
};

int main() {
#if (configUSE_IDLE_HOOK == 1)
  // ExitAfterTestComplete will be called by scheduler in idle task after no
  // user task ready for execute.
  paraos::freertos_idle_fnc_ptr = ExitAfterTestComplete;
#endif

  TestMessage print1{"Thread 1"};
  thread_ptr.push_back(&print1);

  TestMessage print2{"Thread 2"};
  thread_ptr.push_back(&print2);

  TestMessage print3{"Thread 3"};
  thread_ptr.push_back(&print3);

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  return 0;
}
