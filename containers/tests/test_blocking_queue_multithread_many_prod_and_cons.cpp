
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

std::atomic<size_t> total_read_elems_cnt{0};
std::atomic<size_t> total_written_elems_cnt{0};

struct Producer : public paraos::Thread {
  Producer(
      const std::string name = "Producer", size_t stack_depth = 1024,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kIdle)
      : paraos::Thread{name, stack_depth, priority} {}

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
      const std::string name = "Consumer", size_t stack_depth = 1024,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kIdle)
      : paraos::Thread{name, stack_depth, priority} {}

  void Run() override {
    using namespace std::chrono_literals;

    while (true) {
      auto elem = queue.Pop(consumer_waiting_timeout_ms);
      const CriticalSection critical;
      if (elem.size() != 0) {
        ++total_read_elems_cnt;
        if (std::find(elems_vector.begin(), elems_vector.end(), elem) !=
            elems_vector.end()) {
          std::cout << "-- " << Name() << " Got elem from queue: " << elem
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
