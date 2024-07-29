
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

std::vector<int> elems_vector{345, 8924, 1314, 43141, 111,
                              222, 333,  444,  555,   2131};

QueueBlocking<int> queue{7};
std::size_t waiting_timeout_ms = 120;

std::atomic<size_t> total_read_elems_cnt{0};
std::atomic<size_t> total_written_elems_cnt{0};

struct Producer : public paraos::Thread {
  Producer(
      const std::string name = "Producer", size_t stack_depth = 1024,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kIdle)
      : paraos::Thread{name, stack_depth, priority} {}

  void Run() override {
    using namespace std::chrono_literals;
    while (running_condition_) {
      {  // Без критической секции вывод в консоли становится трудным для
        // восприятия...
        const paraos::CriticalSection critical;
        auto vec_idx = total_written_elems_cnt.load();
        if (vec_idx < elems_vector.size()) {
          auto elem = elems_vector[vec_idx];
          std::cout << Name() << " inserting elem:" << elem << std::endl;
          queue.Push(elem, waiting_timeout_ms);
          ++vec_idx;
          total_written_elems_cnt.store(vec_idx);
        } else {
          running_condition_ = false;
        }
      }
      std::this_thread::sleep_for(30ms);
    }
    std::cout << Name() << " Exiting... " << std::endl;
  }

 private:
  bool running_condition_{true};
};

struct Consumer : public paraos::Thread {
  Consumer(
      const std::string name = "Consumer", size_t stack_depth = 1024,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kIdle)
      : paraos::Thread{name, stack_depth, priority} {}

  void Run() override {
    using namespace std::chrono_literals;

    while (running_condition_) {
      auto cnt = 0;
      {
        const paraos::CriticalSection critical;
        cnt = total_read_elems_cnt.load();
      }
      if (cnt < elems_vector.size()) {
        std::cout << Name() << " Popping " << cnt << std::endl;
        auto elem = queue.Pop(waiting_timeout_ms);
        if (elem != 0) {
          if (std::find(elems_vector.begin(), elems_vector.end(), elem) !=
              elems_vector.end()) {
            std::cout << Name() << " Got elem from queue: " << elem
                      << std::endl;

            auto cnt = total_read_elems_cnt.load();
            ++cnt;
            total_read_elems_cnt.store(cnt);
            std::cout << Name() << " Update read counter: " << cnt << std::endl;
          } else {
            assert(false);
          }
        }
      } else {
        running_condition_ = false;
      }

      // Уступить ресурсы другим потокам
      std::this_thread::sleep_for(200ms);
    }
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

  return 0;
}
