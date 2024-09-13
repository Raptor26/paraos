/// @file test_message_multithread_one_producer_many_consumers.cpp
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
#include <cstring>
#include <iostream>
#include <string>
#include <syncstream>
#include <thread>
#include <vector>

#include "paraos_message_buffer.hpp"
#include "paraos_thread.hpp"

using namespace paraos;

/// @brief Burning Heart
const std::vector<std::string> elems_vector{
    "1)  Two worlds collide",
    "2)  Rival nations",
    "3)  It's a primitive clash",
    "4)  Venting years of frustration",
    "5)  Bravely we hope",
    "6)  Against all hope",
    "7)  There is so much at stake",
    "8)  Seems our freedom's up",
    "9)  Against the ropes",
    "10) Does the crowd understand?",
    "11) Is it East versus West",
    "12) Or man against man?",
    "13) Can any nation stand alone?",
    "14) In the burning Heart",
    "15) Just about to burst",
    "16) There's a quest for answers",
    "17) An unquenchable thirst",
    "18) In the darkest night",
    "19) Rising like a spire",
    "20) In the burning heart",
    "21) The unmistakable fire",
    "22) -----------------------------"};

/// @brief Контейнер в который записываются строки, считанные потоками
/// 'Consumer'.
std::vector<std::string> consumers_str_container;

paraos::MessageBuffer message_buff{3};
std::size_t producer_waiting_timeout_ms{1000};
std::size_t consumer_waiting_timeout_ms{10};

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
  if (elems_vector.size() == consumers_str_container.size()) {
    std::cout << "elems_vector.size() == consumers_str_container.size()"
              << std::endl;
    paraos::Thread::DeleteAll();
    elems_vector.~vector();
    consumers_str_container.~vector();
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

      is_message_pushed = false;
      if (str_cnt < elems_vector.size()) {
        auto elem = elems_vector[str_cnt];

        auto message =
            message_buff.Alloc(elem.size() + 1u, producer_waiting_timeout_ms);

        if (message) {
          memcpy(
              message.Addr(), static_cast<const void *>(elem.c_str()),
              message.Size());

          // Принудительно отправить сообщение в очередь, не дожидаясь вызова
          // деструктора
          is_message_pushed = message.Push();
          if (is_message_pushed) {
            const CriticalSection critical;
            std::cout << Name() << " inserting elem: " << elem << std::endl;
          }
        }
      } else {
        // Все данные записаны, необходимо выйти из цикла while
        break;
      }
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
      // Если все данные уже считаны, то необходимо выйти из цикла while и
      // завершить работу
      if (total_read_elems_cnt.load() >= elems_vector.size()) {
        break;
      }

      auto elem = message_buff.Pop(consumer_waiting_timeout_ms);
      if (elem) {
        const CriticalSection critical;
        ++total_read_elems_cnt;

        // Отправить считанную из буфера строку в контейнер чтобы в конце
        // работы программы можно было проверить, что все строки считаны и
        // соответствуют тем данным, которые планировалось
        // записать в буфер из 'elems_vector'.
        consumers_str_container.push_back(
            std::string(static_cast<char *>(elem->Addr())));
        std::cout << "-- " << Name() << " Got elem from message_buff: "
                  << static_cast<char *>(elem->Addr()) << std::endl;
      }
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
      elems_vector.size() == consumers_str_container.size() &&
      "We don't write all strings from 'elems_vector' to "
      "'consumers_str_container'");

  for (auto &str : consumers_str_container) {
    assert(
        std::find(elems_vector.begin(), elems_vector.end(), str) !=
            elems_vector.end() &&
        "Can't find consumer string in source container");
  }

  return 0;
}
