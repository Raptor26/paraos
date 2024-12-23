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
#include "paraos_mutex.hpp"
#include "paraos_mutex_raii.hpp"
#include "paraos_thread.hpp"

using namespace paraos;

#define PrintDebug(__message__)                   \
  {                                               \
    const paraos::CriticalSection macro_critical; \
    std::cout << __message__ << std::endl;        \
  }

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

/// @brief In this container producers write string each written in buffer.
std::vector<std::string> producers_str_container;

paraos::MessageBuffer<3> message_buff;
std::size_t producer_waiting_timeout_ms{1000};
std::size_t consumer_waiting_timeout_ms{10};

std::atomic<std::size_t> total_read_elems_cnt{0};
std::atomic<std::size_t> total_written_elems_cnt{0};

/// @brief String index ready to write in buffer. By test design, value may be
/// more than actual string numb in str_array.
std::atomic_size_t producer_actual_str_idx{0};

std::atomic_size_t consumer_actual_read_str_idx{0};

/// @brief Set actual value in main.
std::size_t thread_total_numb{0};

std::atomic_size_t producer_total_thread_numb{0};
std::atomic_size_t producer_thread_exit_cnt{0};
std::atomic_size_t consumer_thread_exit_cnt{0};
std::atomic_size_t consumer_total_thread_numb{0};

struct Producer : public paraos::Thread {
  Producer(
      const std::string name = "Producer", std::size_t stack_depth = 1024,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kIdle)
      : paraos::Thread{name, stack_depth, priority} {
    SetNeedWhile(true);
    Start();
  }

  /// @brief Producer thread.
  void Run() override {
    std::size_t str_idx;
    {
      const paraos::CriticalSection critical;
      str_idx = producer_actual_str_idx;
      ++producer_actual_str_idx;
    }

    if (str_idx < elems_vector.size()) {
      // Trying to write message in buffer will not work yet.
      while (true) {
        bool is_push_success{false};

        auto write = message_buff.Alloc(elems_vector.at(str_idx).length() + 1u);

        // If memory alloc successful.
        if (write) {
          memcpy(write.Data(), elems_vector.at(str_idx).data(), write.Size());
          is_push_success = write.TryPush();
        }

        if (is_push_success) {
          PrintDebug(
              Name() << " string write successful: "
                     << elems_vector.at(str_idx).c_str());

          paraos::CriticalSection critical;
          producers_str_container.push_back(elems_vector.at(str_idx).c_str());

          break;
        } else {
          PrintDebug(
              Name() << " WARN: Nothin written, try again after delay. "
                     << "String idx is " << str_idx);

          // Small delay for yeld resources.
          Thread::DelayMs(10);
        }

        // No consumers online, nobody read read data from buffer, don't try
        // write data in buffer again.
        if (IsConsumersOffline()) {
          Exit();
          break;
        }
      }
    } else {
      Exit();
    }
  }

 private:
  void Exit() {
    PrintDebug(Name() << " Exiting... ");
    ++producer_thread_exit_cnt;
    SetNeedWhile(false);
  }

  bool IsConsumersOffline() {
    bool is_need_exit{false};

    if (consumer_thread_exit_cnt >= consumer_total_thread_numb) {
      is_need_exit = true;
    }

    return is_need_exit;
  }
};

struct Consumer : public paraos::Thread {
  Consumer(
      const std::string name = "Consumer", std::size_t stack_depth = 1024,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kIdle)
      : paraos::Thread{name, stack_depth, priority} {
    SetNeedWhile(true);
    Start();
  }

  /// @brief Consumers thread.
  void Run() override {
    // Small delay for yeld resources.
    constexpr std::size_t delay_ms{1};

    {
      auto read_message = message_buff.Pop(delay_ms);

      if (read_message) {
        const paraos::CriticalSection critical;

        consumers_str_container.push_back(
            static_cast<char *>(read_message->Data()));

        PrintDebug(
            Name() << " string read successful: "
                   << static_cast<char *>(read_message->Data()));

        read_message.reset();

        ++consumer_actual_read_str_idx;
      } else {
        if (delay_ms == 0) {
          // Small delay for yeld resources.
          Thread::DelayMs(1);
        }
      }
    }

    const CriticalSection critical;
    // If all string read.
    if (consumers_str_container.size() >= elems_vector.size()) {
      // No producers online, nobody write new data, need exit from thread.
      if (IsProducersOffline()) {
        Exit();
      }
    }
  }

 private:
  void Exit() {
    PrintDebug(Name() << " Exiting... ");
    ++producer_thread_exit_cnt;
    SetNeedWhile(false);
  }

  bool IsProducersOffline() {
    bool is_need_exit{false};

    if (producer_thread_exit_cnt >= producer_total_thread_numb) {
      is_need_exit = true;
    }

    return is_need_exit;
  }

 private:
  PARAOS_MAYBE_UNUSED bool running_condition_{true};
};

void CheckIfTestSuccessfullyComplete() {
  const CriticalSection critical;

  PrintDebug(
      "Expected written strings numb is " << elems_vector.size()
                                          << ". Actual written is "
                                          << producers_str_container.size());

  PrintDebug(
      "Expected read strings numb is " << elems_vector.size()
                                       << ". Actual read is "
                                       << consumers_str_container.size());

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

/// FreeRTOS can't stop scheduler. In this case we must manually call
/// exit(EXIT_SUCCESS) after test complete.
#if defined(FREERTOS)
void ExitAfterTestComplete() {
  paraos::CriticalSection critical;
  if ((producer_thread_exit_cnt >= producer_total_thread_numb) &&
      (producer_total_thread_numb >= consumer_total_thread_numb)) {
    CheckIfTestSuccessfullyComplete();
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

  Consumer elem_consumer_1{
      "--Consumer 1", 1024u, paraos::ThreadPriority::kLowest};
  consumer_total_thread_numb += 1;
  Consumer elem_consumer_2{
      "--Consumer 2", 1024u, paraos::ThreadPriority::kBelowNormal};
  consumer_total_thread_numb += 1;
  Consumer elem_consumer_3{
      "--Consumer 3", 1024u, paraos::ThreadPriority::kRealTime};
  consumer_total_thread_numb += 1;

  // ---------------------------------------------------------------------------
  // Producers Init
  // ---------------------------------------------------------------------------
  Producer elem_producer_1{
      "Producer 1", 1024u, paraos::ThreadPriority::kLowest};
  producer_total_thread_numb += 1;

  Producer elem_producer_2{
      "Producer 2", 1024u, paraos::ThreadPriority::kBelowNormal};
  producer_total_thread_numb += 1;

  Producer elem_producer_3{
      "Producer 3", 1024u, paraos::ThreadPriority::kNormal};
  producer_total_thread_numb += 1;

  Producer elem_producer_4{
      "Producer 4", 1024u, paraos::ThreadPriority::kAboveNormal};
  producer_total_thread_numb += 1;

  Producer elem_producer_5{
      "Producer 5", 1024u, paraos::ThreadPriority::kHighest};
  producer_total_thread_numb += 1;

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  CheckIfTestSuccessfullyComplete();

  return 0;
}
