/// @file test_queue_multithread_one_producer_many_consumers.cpp
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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <syncstream>
#include <thread>
#include <vector>

#include "paraos_queue.hpp"
#include "paraos_thread.hpp"

using namespace paraos;

/// @brief That’s All - Genesis
std::vector<std::string> song_str{
    "1)  Just as I thought it was goin' alright",
    "2)  I found out I'm wrong, when I thought I was right",
    "3)  It's always the same, it's just a shame, that's all",
    "4)  I could say day, and you'd say night",
    "5)  Tell me it's black, when I know that it's white",
    "6)  Always the same, it's just a shame and that's all",
    "7)  I could leave, but I won't go",
    "8)  Though my heart might tell me so",
    "9)  I can't feel a thing from my head down to my toes",
    "10) But why does it always seem to be",
    "11) Me lookin' at you, you lookin' at me",
    "12) It's always the same, it's just a shame, that's all",
    "13) ---------------------------------------------------"};

Queue<std::string> queue{100};

std::atomic<std::size_t> total_read_str_cnt{0};

std::size_t thread_total_numb{0};
std::size_t thread_exit_cnt{0};

#if defined(__linux__) && defined(freeRTOS)
#define configUSE_IDLE_HOOK 1
#include <stdlib.h>
static bool threads_deleted_flag = false;
/// @brief The idle task runs at the very lowest priority, so such an idle hook
/// function will only get executed when there are no tasks of higher priority
/// that are able to run.
extern "C" void vApplicationIdleHook(void) {
  if (threads_deleted_flag) {
    std::cout << "Exiting program..." << std::endl;
    _Exit(0);
  }
  if (total_read_str_cnt == song_str.size()) {
    std::cout << "total_read_str_cnt == song_str.size()" << std::endl;
    // paraos::Thread::DeleteAll();
    // queue.~Queue();
    song_str.~vector();
    total_read_str_cnt.~atomic();
    threads_deleted_flag = true;
  }
}
#endif

struct Producer : public paraos::Thread {
  Producer(
      const std::string name = "Producer", std::size_t stack_depth = 1024,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kLowest)
      : paraos::Thread{name, stack_depth, priority} {
    Start();
  }

  void Run() override {
    while (song_cnt < song_str.size()) {
      std::cout << Name() << " str:" << song_str[song_cnt] << std::endl;

      {
        const paraos::CriticalSection critical;
        queue.Push(song_str[song_cnt]);
      }
      ++song_cnt;
    }

    const CriticalSection critical;
    ++thread_exit_cnt;
  }

 private:
  std::size_t song_cnt{0};
};

struct Consumer : public paraos::Thread {
  Consumer(
      const std::string name = "Consumer", std::size_t stack_depth = 1024,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kLowest)
      : paraos::Thread{name, stack_depth, priority} {
    Start();
  }

  void Run() override {
    using namespace std::chrono_literals;

    while (!is_read_str) {
      // Если все строки уже считаны
      if (total_read_str_cnt.load() >= song_str.size()) {
        is_read_str = true;
      } else {
        std::optional<std::string> str;
        {
          const paraos::CriticalSection critical;
          str = queue.Pop();
        }
        if (str) {
          auto cnt = total_read_str_cnt.load();
          ++cnt;
          total_read_str_cnt.store(cnt);
          if (std::find(song_str.begin(), song_str.end(), *str) !=
              song_str.end()) {
            std::cout << Name() << " str: " << *str << std::endl;
          } else {
            assert(false);
          }
        }

      }  // out critical section

      // Уступить ресурсы другим потокам
      std::this_thread::sleep_for(1ms);
    }

    const CriticalSection critical;
    ++thread_exit_cnt;
  }

 private:
  bool is_read_str{false};
};

/// FreeRTOS can't stop scheduler. In this case we must manually call
/// exit(EXIT_SUCCESS) after test complete.
#if defined(FREERTOS)
void ExitAfterTestComplete() {
  auto is_need_exit{false};
  {
    paraos::CriticalSection critical;
    if (thread_exit_cnt == thread_total_numb) {
      is_need_exit = true;
    }
  }

  if (is_need_exit) {
    exit(EXIT_SUCCESS);
  }
}
#endif

int main() {
#if defined(FREERTOS)
  // ExitAfterTestComplete will be called by scheduler in idle task after no
  // user task ready for execute.
  paraos::freertos_idle_fnc_ptr = ExitAfterTestComplete;
#endif

  Consumer str_consumer_1{"Consumer 1", 1024u, paraos::ThreadPriority::kLowest};
  Consumer str_consumer_2{
      "Consumer 2", 1024u, paraos::ThreadPriority::kBelowNormal};
  Consumer str_consumer_3{
      "Consumer 3", 1024u, paraos::ThreadPriority::kBelowNormal};
  Consumer str_consumer_4{"Consumer 4", 1024u, paraos::ThreadPriority::kNormal};
  Consumer str_consumer_5{"Consumer 5", 1024u, paraos::ThreadPriority::kNormal};
  thread_total_numb += 5;

  Producer str_producer{"Producer 1", 1024u, paraos::ThreadPriority::kNormal};
  thread_total_numb += 1;

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  assert(
      total_read_str_cnt == song_str.size() &&
      "Consumers don't read all strings from source container");

  assert(
      thread_total_numb == thread_exit_cnt &&
      "Consumers don't read all strings from source container");

  return 0;
}