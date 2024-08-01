
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

struct Producer : public paraos::Thread {
  Producer(
      const std::string name = "Producer", std::size_t stack_depth = 1024,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kIdle)
      : paraos::Thread{name, stack_depth, priority} {}

  void Run() override {
    while (song_cnt < song_str.size()) {
      const paraos::CriticalSection critical;
      std::cout << Name() << " str:" << song_str[song_cnt] << std::endl;

      queue.Push(song_str[song_cnt]);
      ++song_cnt;
    }
  }

 private:
  std::size_t song_cnt{0};
};

struct Consumer : public paraos::Thread {
  Consumer(
      const std::string name = "Consumer", std::size_t stack_depth = 1024,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kIdle)
      : paraos::Thread{std::move(name), stack_depth, priority} {}

  void Run() override {
    using namespace std::chrono_literals;

    while (!is_read_str) {
      // Если все строки уже считаны
      if (total_read_str_cnt.load() >= song_str.size()) {
        is_read_str = true;
      } else {
        // todo Удалить строку ниже, атомарность должна обеспечиваться очередью
        const paraos::CriticalSection critical;

        auto str = queue.Pop();
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
  }

 private:
  bool is_read_str{false};
};

int main() {
  Consumer str_consumer_1{"Consumer 1", 1024u, paraos::ThreadPriority::kLowest};
  Consumer str_consumer_2{
      "Consumer 2", 1024u, paraos::ThreadPriority::kBelowNormal};
  Consumer str_consumer_3{"Consumer 3", 1024u, paraos::ThreadPriority::kNormal};
  Consumer str_consumer_4{
      "Consumer 4", 1024u, paraos::ThreadPriority::kAboveNormal};
  Consumer str_consumer_5{
      "Consumer 5", 1024u, paraos::ThreadPriority::kHighest};

  Producer str_producer{};

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  return 0;
}