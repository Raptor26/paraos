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

// NOLINTBEGIN(misc-include-cleaner, readability-magic-numbers)
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "paraos_config.hpp"
#include "paraos_critical.hpp"
#include "paraos_message_buffer.hpp"
#include "paraos_thread.hpp"
#include "paraos_thread_common.hpp"

#define PrintDebug(__message__, __object_name__)               \
  {                                                            \
    const paraos::CriticalSection macro_critical;              \
                                                               \
    const std::time_t result = std::time(nullptr);             \
                                                               \
    std::cout << "Time: '" << result << " " << __object_name__ \
              << "': " << __message__ << "\n";                 \
  }

/// @brief Burning Heart
const std::vector<std::string> elems_vector{
    "1)  Two worlds collide",     "2)  Rival nations",
    "3)  It's a primitive clash", "4)  Venting years of frustration",
    "5)  Bravely we hope",        "6)  Against all hope",
};

namespace {
paraos::Thread check_test_complete_and_exit{paraos::ThreadAttr{
    "Check test complete", paraos::GetStackMinimumSizeInBytes(),
    paraos::ThreadPriority::kRealTime, nullptr}};

/// @brief Контейнер в который записываются строки, считанные потоками
/// 'Consumer'.
std::vector<std::string> consumers_str_container;

/// @brief In this container producers write string each written in buffer.
std::vector<std::string> producers_str_container;

paraos::MessageBuffer<3> message_buff;

constexpr std::size_t producer_waiting_timeout_ms{10};
constexpr std::size_t consumer_waiting_timeout_ms{5};

std::atomic_size_t producer_total_thread_numb{0};
std::atomic_size_t producer_thread_exit_cnt{0};
std::atomic_size_t consumer_thread_exit_cnt{0};
std::atomic_size_t consumer_total_thread_numb{0};
}  // namespace

struct Producer {
  explicit Producer(const paraos::ThreadAttr &attr, std::size_t str_idx)
      : thread_{attr}, str_idx_{str_idx} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<Producer, &Producer::Run>(*this));
  }

  /// @brief Producer thread.
  void Run() {
    if (str_idx_ < elems_vector.size()) {
      // Trying to write message in buffer will not work yet.
      while (true) {
        bool is_push_success{false};

        auto write =
            message_buff.Alloc(elems_vector.at(str_idx_).length() + 1U);

        // If memory alloc successful.
        if (write) {
          memcpy(write.Data(), elems_vector.at(str_idx_).data(), write.Size());
          is_push_success = write.TryPush();
        }

        if (is_push_success) {
          PrintDebug(
              " string write successful: " << elems_vector.at(str_idx_).c_str(),
              thread_.GiveName());

          const paraos::CriticalSection critical;
          producers_str_container.emplace_back(
              elems_vector.at(str_idx_).c_str());

          Exit();
          break;
        }
        PrintDebug(
            " WARN: Nothin written, try again after delay. " << "String idx is "
                                                             << str_idx_,
            thread_.GiveName());

        // Small delay for yeld resources.
        paraos::Thread::DelayMs(producer_waiting_timeout_ms);
      }
    } else {
      Exit();
    }
  }

 private:
  void Exit() {
    ++producer_thread_exit_cnt;
    PrintDebug(" exiting ... ", thread_.GiveName());
    thread_.Finished();
  }

  paraos::Thread thread_;
  std::size_t str_idx_{1000};
};

struct Consumer {
  explicit Consumer(const paraos::ThreadAttr &attr, std::size_t thread_id)
      : thread_{attr}, thread_id_{thread_id} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<Consumer, &Consumer::Run>(*this));
  }

  /// @brief Consumers thread.
  void Run() {
    // Small delay for yeld resources.
    constexpr std::size_t delay_ms{1};

    {
      auto read_message = message_buff.Pop(delay_ms);

      if (read_message) {
        const paraos::CriticalSection critical;

        consumers_str_container.emplace_back(
            static_cast<char *>(read_message->Data()));

        PrintDebug(
            " string read successful: "
                << static_cast<char *>(read_message->Data()),
            thread_.GiveName());

        read_message.reset();

        Exit();
      } else {
        paraos::Thread::DelayMs(consumer_waiting_timeout_ms);
      }
    }
  }

 private:
  void Exit() {
    ++consumer_thread_exit_cnt;
    PrintDebug(" exiting ... ", thread_.GiveName());
    thread_.Finished();
  }

 private:
  paraos::Thread thread_;

  const std::size_t thread_id_;
};

namespace {
void CheckIfTestSuccessfullyComplete() {
  const paraos::CriticalSection critical;

  PrintDebug(
      "Expected written strings numb is " << elems_vector.size()
                                          << ". Actual written is "
                                          << producers_str_container.size(),
      "CheckIfTestSuccessfullyComplete()");

  PrintDebug(
      "Expected read strings numb is " << elems_vector.size()
                                       << ". Actual read is "
                                       << consumers_str_container.size(),
      "CheckIfTestSuccessfullyComplete()");

  // Each string must be read by consumers.
  assert(
      elems_vector.size() == consumers_str_container.size() &&
      "We don't write all strings from 'elems_vector' to "
      "'consumers_str_container'");

  // Check each string in consumers_str_container contained in elems_vector.
  for (auto &str : consumers_str_container) {
    assert(
        std::find(elems_vector.begin(), elems_vector.end(), str) !=
            elems_vector.end() &&
        "Can't find consumer string in source container");
  }
}

void ExitFromTest() {
  bool is_test_complete{false};
  {
    const paraos::CriticalSection critical;
    if (consumers_str_container.size() >= elems_vector.size()) {
      is_test_complete = true;
    }
  }

  if (is_test_complete) {
    check_test_complete_and_exit.Finished();

    constexpr std::size_t delay_ms{0};
    PrintDebug("Ready to exit, delay ms " << delay_ms, "ExitFromTest");
    paraos::Thread::DelayMs(delay_ms);

    CheckIfTestSuccessfullyComplete();

    PrintDebug("Call paraos::Thread::Exit();", "ExitFromTest");

#if defined(PARAOS_LIKE_FREERTOS)
    // Forces program exit to reduce execution time. Needed to terminate tests
    // early, especially when running multiple tests. In other case, program
    // will exit in 1 second later.
    std::_Exit(EXIT_SUCCESS);
#else
    paraos::Thread::Exit();
#endif
  }

  PrintDebug("Yeld resources", "ExitFromTest");
  paraos::Thread::DelayMs(10);
}
}  // namespace

auto main() -> int {
  {
    static auto delegate = etl::delegate<void()>::create<ExitFromTest>();
    check_test_complete_and_exit.RegisterDelegate(delegate);
  }

  // ---------------------------------------------------------------------------
  // Consumers Init
  // ---------------------------------------------------------------------------
  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 0";
    attr.priority = paraos::ThreadPriority::kLowest;
    const static Consumer cons_0{attr, 0};
    consumer_total_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 1";
    attr.priority = paraos::ThreadPriority::kBelowNormal;
    const static Consumer cons_1{attr, 1};
    consumer_total_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 2";
    attr.priority = paraos::ThreadPriority::kNormal;
    const static Consumer cons_2{attr, 2};
    consumer_total_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 3";
    attr.priority = paraos::ThreadPriority::kNormal;
    const static Consumer cons_3{attr, 2};
    consumer_total_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 4";
    attr.priority = paraos::ThreadPriority::kNormal;
    const static Consumer cons_4{attr, 2};
    consumer_total_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 5";
    attr.priority = paraos::ThreadPriority::kLowest;
    const static Consumer cons_5{attr, 0};
    consumer_total_thread_numb += 1;
  }

  // ---------------------------------------------------------------------------
  // Producers Init
  // ---------------------------------------------------------------------------
  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 0";
    attr.priority = paraos::ThreadPriority::kAboveNormal;
    const static Producer prod_0{attr, 0};
    producer_total_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 1";
    attr.priority = paraos::ThreadPriority::kHighest;
    const static Producer prod_1{attr, 1};
    producer_total_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 2";
    const static Producer prod_2{attr, 2};
    producer_total_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 3";
    const static Producer prod_3{attr, 3};
    producer_total_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 4";
    const static Producer prod_4{attr, 4};
    producer_total_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 5";
    attr.priority = paraos::ThreadPriority::kAboveNormal;
    const static Producer prod_5{attr, 5};
    producer_total_thread_numb += 1;
  }

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  return 0;
}
// NOLINTEND(misc-include-cleaner, readability-magic-numbers)
