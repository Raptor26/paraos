/// @file test_blocking_queue_multithread_many_prod_and_cons.cpp
/// @author VyhodcevEgor <vyhodcev@internet.ru>
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

#include "paraos_queue_blocking.hpp"
#include "paraos_thread.hpp"

using namespace paraos;

const std::vector<std::string> elems_vector{"1)", "2)", "3)", "4)", "5)",
                                            "6)", "7)", "8)", "9)", "10)"};

QueueBlocking<std::string> queue{2};
std::size_t producer_waiting_timeout_ms{5000};
std::size_t consumer_waiting_timeout_ms{50};

std::atomic<std::size_t> total_read_elems_cnt{0};
std::atomic<std::size_t> total_written_elems_cnt{0};

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
  if (total_written_elems_cnt == total_read_elems_cnt) {
    std::cout << "total_written_elems_cnt == total_read_elems_cnt "
              << std::endl;
    elems_vector.~vector();
    total_read_elems_cnt.~atomic();
    total_written_elems_cnt.~atomic();
    threads_deleted_flag = true;
  }
}
#endif

struct Producer : public paraos::Thread {
  Producer(
      const std::string name = "Producer", std::size_t stack_depth = 1024,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kIdle)
      : paraos::Thread{name, stack_depth, priority} {
    Start();
  }

  void Run() override {
    bool is_message_pushed{true};
    std::size_t str_cnt{0};
    using namespace std::chrono_literals;
    while (true) {
      if (is_message_pushed) {
        const CriticalSection critical;
        // Мы должны атомарно определить строку, которую будем записывать в
        // буфер сообщений на данной итерации цикла while текущего потока.
        // После, несколько потоков могут параллельно записывать разные строки
        // в буфер сообщений без состояния гонки.

        str_cnt = total_written_elems_cnt.load();
        if (str_cnt < elems_vector.size()) {
          // Инкрементируем счетчик чтобы другой поток (или данный, но уже на
          // следующей итерации цикла while) "взял" следующую строку для
          // записи в буфер.
          ++total_written_elems_cnt;
        }
      }

      if (str_cnt < elems_vector.size()) {
        auto elem = elems_vector[str_cnt];

        if (queue.Push(elem, producer_waiting_timeout_ms)) {
          const CriticalSection critical;
          std::cout << Name() << " inserting elem: " << elem << std::endl;
        } else {
          assert(false && "Can't push element in queue");
        }
        is_message_pushed = true;
      } else {
        break;
      }

      std::this_thread::sleep_for(1ms);
    }
    const CriticalSection critical;
    std::cout << Name() << " Exiting... " << std::endl;
  }
};

struct Consumer : public paraos::Thread {
  Consumer(
      const std::string name = "Consumer", std::size_t stack_depth = 1024,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kIdle)
      : paraos::Thread{name, stack_depth, priority} {
    Start();
  }

  void Run() override {
    using namespace std::chrono_literals;

    while (true) {
      auto elem = queue.Pop(consumer_waiting_timeout_ms);
      const CriticalSection critical;
      if (elem) {
        ++total_read_elems_cnt;
        if (std::find(elems_vector.begin(), elems_vector.end(), *elem) !=
            elems_vector.end()) {
          std::cout << "-- " << Name() << " Got elem from queue: " << *elem
                    << std::endl;
        } else {
          assert(false);
        }
      }

      if (total_read_elems_cnt.load() >= elems_vector.size()) {
        break;
      }

      // Уступить ресурсы другим потокам
      std::this_thread::sleep_for(1ms);
    }
    const CriticalSection critical;
    std::cout << Name() << " Exiting... " << std::endl;
  }

 private:
  bool running_condition_{true};
};

int main() {
  Consumer elem_consumer_1{
      "Consumer 1", 1024u, paraos::ThreadPriority::kLowest};
  Consumer elem_consumer_2{
      "Consumer 2", 1024u, paraos::ThreadPriority::kBelowNormal};
  Consumer elem_consumer_3{
      "Consumer 3", 1024u, paraos::ThreadPriority::kNormal};

  Producer elem_producer_1{
      "Producer 1", 1024u, paraos::ThreadPriority::kLowest};
  Producer elem_producer_2{
      "Producer 2", 1024u, paraos::ThreadPriority::kNormal};
  Producer elem_producer_3{
      "Producer 3", 1024u, paraos::ThreadPriority::kNormal};
  Producer elem_producer_4{
      "Producer 4", 1024u, paraos::ThreadPriority::kBelowNormal};

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  const CriticalSection critical;
  std::cout << "Total write elements is " << total_written_elems_cnt
            << std::endl;
  assert(
      elems_vector.size() == total_written_elems_cnt &&
      "Miss some writable element");

  assert(
      elems_vector.size() == total_read_elems_cnt &&
      "Miss some readable element");

  return 0;
}
