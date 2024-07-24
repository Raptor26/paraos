
#include <windows.h>

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

std::atomic<size_t> total_read_str_cnt{0};

struct Producer : public ThreadBase {
  Producer() : ThreadBase{false} {}

  void Processing() override {
    while (song_cnt < song_str.size()) {
      const paraos::CriticalSection critical;
      std::cout << song_str[song_cnt] << std::endl;

      queue.Push(song_str[song_cnt]);
      ++song_cnt;
    }
  }

 private:
  size_t song_cnt{0};
};

struct Consumer : public ThreadBase {
  Consumer(size_t numb) : ThreadBase{false}, number_{numb} {}

  void Processing() override {
    using namespace std::chrono_literals;

    while (!is_read_str) {
      // Если все строки уже считаны
      if (total_read_str_cnt.load() >= song_str.size()) {
        is_read_str = true;
      } else {
        // todo Удалить строку ниже, ароматность должна обеспечиваться очередью
        const paraos::CriticalSection critical;

        if (!queue.IsEmpty()) {
          auto str = queue.Pop();
          if (!str.empty()) {
            auto cnt = total_read_str_cnt.load();
            ++cnt;
            total_read_str_cnt.store(cnt);
            if (std::find(song_str.begin(), song_str.end(), str) !=
                song_str.end()) {
              std::cout << "Read consumer numb " << number_ << " str: " << str
                        << std::endl;
            } else {
              assert(false);
            }
          }
        }
      }  // out critical section

      // Уступить ресурсы другим потребителям
      std::this_thread::sleep_for(1ms);
    }
  }

 private:
  bool is_read_str{false};
  size_t number_;
};

int main() {
  Consumer str_consumer_1{1};
  Consumer str_consumer_2{2};
  Consumer str_consumer_3{3};
  Consumer str_consumer_4{4};
  Consumer str_consumer_5{5};
  Consumer str_consumer_6{6};
  Producer str_producer;

  ThreadFactory.Make(str_consumer_1);
  ThreadFactory.Make(str_consumer_2);
  ThreadFactory.Make(str_consumer_3);
  ThreadFactory.Make(str_consumer_4);
  ThreadFactory.Make(str_consumer_5);
  ThreadFactory.Make(str_consumer_6);
  ThreadFactory.Make(str_producer);

  ThreadFactory.StartScheduler();
  return 0;
}