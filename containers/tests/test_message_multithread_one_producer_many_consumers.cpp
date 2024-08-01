
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <syncstream>
#include <thread>
#include <vector>

#include "paraos_message_buffer.hpp"
#include "paraos_thread.hpp"
#include "paraos_trace.hpp"

using namespace paraos;

/// @brief Burning Heart
const std::vector<std::string> song_str{
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
    "14) -----------------------------"};

using namespace std;

// mutex to block threads
mutex mtx;
mutex mtx_consumer;
condition_variable cv;

constexpr std::size_t message_buff_capacity{1};

MessageBuffer message_buff{message_buff_capacity};

std::atomic<std::size_t> consumer_str_cnt{0};
std::atomic<std::size_t> producer_str_cnt{0};
std::size_t total_write_str_cnt{0};

class MessageProducer final : public Thread {
 public:
  MessageProducer(
      const std::string name = "Message producer thread",
      std::size_t stack_depth = 1024,
      ThreadPriority priority = ThreadPriority::kLowest)
      : Thread{name, stack_depth, priority} {}

  ~MessageProducer() = default;

  void Run() override {
    using namespace std::chrono_literals;

    std::size_t str_cnt{0};
    std::size_t write_str_cnt{0};
    std::size_t delay_ms{1000};
    std::size_t try_cnt{0};
    constexpr std::size_t try_cnt_wax{10};
    bool is_message_pushed{true};
    while (true) {
      if (is_message_pushed) {
        // Мы должны атомарно определить строку, которую будем записывать в
        // буфер сообщений на данной итерации цикла while текущего потока.
        // После, несколько потоков могут параллельно записывать разные строки
        // в буфер сообщений без состояния гонки.
        const CriticalSection critical;
        str_cnt = producer_str_cnt.load();
        if (str_cnt < song_str.size()) {
          // Инкрементируем счетчик чтобы другой поток (или данный, но уже на
          // следующей итерации цикла while) "взял" следующую строку для
          // записи в буфер.
          ++producer_str_cnt;
        }
      }

      is_message_pushed = false;
      if (str_cnt < song_str.size()) {
        // lock_guard<mutex> lock(mtx);

        auto str_len_with_terminate_symbol = song_str.at(str_cnt).length() + 1u;
        auto message =
            message_buff.Alloc(str_len_with_terminate_symbol, delay_ms);

        if (message) {
          {
            const CriticalSection critical;
            auto addr = static_cast<char*>(message.GetAddr());

            strncpy(addr, song_str.at(str_cnt).c_str(), message.GetSize());
          }

          //   std::cout << "Buffer size before push is " <<
          //   message_buff.Size()
          //             << std::endl;
          is_message_pushed = message.Push(delay_ms);
          if (is_message_pushed) {
            // const CriticalSection critical;
            std::cout << Name() << " str: " << song_str.at(str_cnt).c_str()
                      << std::endl;

            ++write_str_cnt;
          } else {
            ++try_cnt;
            std::cout << "!!! " << Name() << ": "
                      << "Can't push in buffer this string: "
                      << song_str.at(str_cnt) << std::endl;
          }
        } else {
          ++try_cnt;
        }

        if (try_cnt > try_cnt_wax) {
          const CriticalSection critical;
          total_write_str_cnt += write_str_cnt;
          break;
        }
      } else {
        const CriticalSection critical;
        total_write_str_cnt += write_str_cnt;
        // Все строки уже записаны, необходимо выйти из цикла while
        break;
      }

      // Принудительно уступить ресурсы другим потокам
      std::this_thread::sleep_for(1ms);
    }  // while (true)

    paraosTRACE_MESSAGE("Producer exit thread ...");
  }
};

class MessageConsumer final : public Thread {
 public:
  MessageConsumer(
      const std::string name = "Message consumer thread",
      std::size_t stack_depth = 1024,
      ThreadPriority priority = ThreadPriority::kLowest)
      : Thread{name, stack_depth, priority} {}

  ~MessageConsumer() = default;

  void Run() override {
    std::size_t delay_ms{10};
    std::size_t try_cnt{0};
    std::size_t read_str_cnt{0};
    constexpr std::size_t try_cnt_wax{5};
    while (true) {
      {
        // lock_guard<mutex> lock(mtx_consumer);
        auto message = message_buff.Pop(delay_ms);

        if (message) {
          const CriticalSection critical;

          auto str = static_cast<char*>(message.GetAddr());

          if (std::find(song_str.begin(), song_str.end(), str) !=
              song_str.end()) {
            ++read_str_cnt;
            std::cout << "-- " << Name() << " str: " << str << std::endl;
            // std::cout << "-- Consumers read string numb is "
            //           << consumer_str_cnt.load() << std::endl;
          } else {
            assert(false && "Can't find string in array");
          }
        } else {
          ++try_cnt;

          // no data in buffer
          break;
        }
      }

      const CriticalSection critical;
      if (consumer_str_cnt >= song_str.size()) {
        break;
      }

      if (try_cnt >= try_cnt_wax) {
        break;
      }

      // Уступить ресурсы другим потокам
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(10ms);
    }

    const CriticalSection critical;
    consumer_str_cnt += read_str_cnt;

    paraosTRACE_MESSAGE("Consumer exit thread ...");
  }
};

int main() {
  MessageProducer producer1{"Producer 1"};
  //   MessageProducer producer2{"Producer 2"};

  MessageConsumer consumer1{"Consumer 1"};
  //   MessageConsumer consumer2{"Consumer 2"};
  //   MessageConsumer consumer3{"Consumer 3"};
  //   MessageConsumer consumer4{"Consumer 4"};

  Thread::StartScheduler();
  Thread::DeleteAll();

#if 1
  std::cout << "Total write string numb is " << total_write_str_cnt
            << std::endl;
  assert(
      total_write_str_cnt == song_str.size() &&
      "Producer don't write all strings");

  std::cout << "Total read string numb is " << consumer_str_cnt << std::endl;
  assert(
      consumer_str_cnt == song_str.size() && "Consumer don't read all strings");
#endif

  return 0;
}