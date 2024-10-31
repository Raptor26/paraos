/// @file test_multi_ringbuff_mpmc.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
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
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "etl/cyclic_value.h"
#include "paraos_bool_atomic.hpp"
#include "paraos_critical.hpp"
#include "paraos_multi_ringbuff.hpp"
#include "paraos_thread.hpp"
#include "paraos_utils.hpp"

#define PrintDebug(__message__)                   \
  {                                               \
    const paraos::CriticalSection macro_critical; \
    std::cout << __message__ << std::endl;        \
  }

const std::vector<std::string> str_array{
    "1) We're talking away",
    "2) I don't know what I'm to say",
    "3) I'll say it anyway",
    "4) Today is another day to find you",
    "5) Shyin' away",
    "6) Oh, I'll be comin' for your love, okay",
    "7) Take on me",
    "8) (Take on me)",
    "9) Take me on",
    "10) (Take on me)",
    "11) I'll be gone",
    "12) In a day or two",
    "13) So needless to say",
    "14) I'm odds and ends",
    "15) But I'll be stumblin' away",
    "16) Slowly learnin' that life is okay",
    "17) Say after me",
    "18) It's no better to be safe than sorry",
    "19) All the things that you say, yeah",
    "20) Is it life or just to play my worries away?",
    "21) You're all the things I've got to remember",
    "22) You're shyin' away",
    "23) I'll be comin' for you anyway",
    "24) -------------------------------------------"};

std::size_t CalcTotalBytesInStringArray(
    const std::vector<std::string> &str_arr) {
  std::size_t total_bytes_numb{0};

  for (auto &str : str_arr) {
    total_bytes_numb += str.length();
  }
  return total_bytes_numb;
}

constexpr std::size_t queue_size{2};
constexpr std::size_t ring_buff_size{2048};
constexpr std::size_t thread_stack_depth{1024};

/// @brief String index ready to write in buffer. By test design, value may be
/// more than actual string numb in str_array.
std::atomic_size_t producer_actual_str_idx{0};

/// @brief After each successful write, producer increment this value.
std::atomic_size_t producer_total_written_bytes{0};

/// @brief  After each successful read, consumer increment this value.
std::atomic_size_t consumer_total_read_bytes{0};

/// ----------------------------------------------------------------------------
/// Variable above need in FreeRTOS port for check conditions when need call
/// exit().
/// ----------------------------------------------------------------------------

std::atomic_size_t consumer_thread_numb{0};

std::atomic_size_t producer_thread_numb{0};

std::atomic_size_t producer_thread_exit_cnt{0};

std::atomic_size_t consumer_thread_exit_cnt{0};
/// ----------------------------------------------------------------------------

paraos::MultiRingBuff<
    queue_size, char, paraos::RingBuff<char, ring_buff_size>,
    paraos::RingBuff<char, ring_buff_size>,
    paraos::RingBuff<char, ring_buff_size>,
    paraos::RingBuff<char, ring_buff_size>,
    paraos::RingBuff<char, ring_buff_size>>
    multi_ring_buff{};

struct Producer : public paraos::Thread {
  Producer(
      const std::size_t thread_id, const std::string name = "Producer",
      std::size_t stack_depth = thread_stack_depth,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kLowest)
      : paraos::Thread{name, stack_depth, priority}, thread_id_{thread_id} {
    paraos::Thread::SetNeedWhile(true);
    Start();
  }

  /// @brief Producer thread
  void Run() override {
    std::size_t str_idx;
    {
      const paraos::CriticalSection critical;
      str_idx = producer_actual_str_idx;
      ++producer_actual_str_idx;
    }

    while (true) {
      if (str_idx < str_array.size()) {
      } else {
        // All data already written, exit from thread.
        ThreadExit();
        break;
      }

      // try write data in buffer periodical.
      auto written_len = multi_ring_buff.TryWrite(
          buff_idx_, str_array.at(str_idx).c_str(),
          str_array.at(str_idx).length());

      if (written_len > 0) {
        // Break trying write data in buff, in next iteration take new string
        // idx for write in buff.
        producer_total_written_bytes += written_len;
        PrintDebug(
            Name() << " string write successful: "
                   << str_array.at(str_idx).c_str());
        break;
      } else {
        PrintDebug(
            Name() << "WARN: Nothin written, try again after delay. "
                   << "String idx is " << str_idx);

        // If no consumers online, nobody read data from buffer and buffer
        // always will full.
        if (IsConsumersOffline()) {
          ThreadExit();
          break;
        }

        // Small delay for yeld resources.
        Thread::DelayMs(1);
      }
    }

    // cyclic increment buff idx.
    ++buff_idx_;
  }

 private:
  void ThreadExit() {
    SetNeedWhile(false);
    ++producer_thread_exit_cnt;
    PrintDebug(Name() << " exiting ... ");
  }

  bool IsConsumersOffline() {
    bool is_need_exit{false};

    if (consumer_thread_exit_cnt >= consumer_thread_numb) {
      is_need_exit = true;
    }

    return is_need_exit;
  }

 private:
  const std::size_t thread_id_;
  etl::cyclic_value<int, 0, multi_ring_buff.GetBuffNumb() - 1u> buff_idx_{0};
};

struct Consumer : public paraos::Thread {
  Consumer(
      const std::string name = "Consumer",
      std::size_t stack_depth = thread_stack_depth,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kRealTime)
      : paraos::Thread{name, stack_depth, priority} {
    paraos::Thread::SetNeedWhile(true);
    Start();
  }

  /// @brief Consumer thread.
  void Run() override {
    // Small delay for yeld recourses if no data available in buff.
    constexpr std::size_t delay_ms{1};
    constexpr std::size_t read_mem_size{2048};
    std::size_t idx;
    auto read_mem = std::make_unique<std::array<char, read_mem_size>>();

    auto read_size =
        multi_ring_buff.Read(idx, read_mem->data(), read_mem->size(), delay_ms);

    if (read_size > 0u) {
      consumer_total_read_bytes += read_size;
      PrintDebug(Name() << " read " << read_mem->data());
    } else {
      PrintDebug(
          Name() << " Nothing read, try again. Already read total bytes is "
                 << consumer_total_read_bytes);
    }

    // No producers online, nobody write new data, need exit from thread.
    if (IsProducersOffline()) {
      ThreadExit();
    }
  }

 private:
  void ThreadExit() {
    SetNeedWhile(false);
    ++consumer_thread_exit_cnt;
    PrintDebug(Name() << " exiting ... ");
  }

  bool IsProducersOffline() {
    bool is_offline{true};
    if (producer_thread_exit_cnt >= producer_thread_numb) {
      is_offline = true;
    }

    return is_offline;
  }
};

void AssertsForTestComplete() {
  constexpr std::size_t read_mem_size{2048};
  std::size_t idx;

  auto read_mem = std::make_unique<std::array<char, read_mem_size>>();

  for (std::size_t i = 0u; i < multi_ring_buff.GetBuffNumb(); ++i) {
    auto read_bytes_numb =
        multi_ring_buff.TryRead(idx, read_mem->data(), read_mem->size());

    if (read_bytes_numb > 0) {
      consumer_total_read_bytes += read_bytes_numb;
      std::cout << "Read some data befor after all threads works complete"
                << std::endl;
    }
  }

  std::cout << "Total read bytes numb is " << consumer_total_read_bytes
            << " Expected bytes numb is "
            << CalcTotalBytesInStringArray(str_array) << std::endl;

  PARAOS_CHECK_ASSERT(
      consumer_total_read_bytes == producer_total_written_bytes &&
      "Total read and written bytes not equal");
}

#if defined(FREERTOS)
void ExitAfterTestComplete() {
  auto is_need_exit{false};
  {
    paraos::CriticalSection critical;
    if ((consumer_thread_exit_cnt >= consumer_thread_numb) &&
        (producer_thread_exit_cnt >= producer_thread_numb)) {
      is_need_exit = true;
    }
  }

  if (is_need_exit) {
    AssertsForTestComplete();
    exit(EXIT_SUCCESS);
  }
}
#endif

auto main() -> int {
#if defined(FREERTOS)
  // ExitAfterTestComplete will be called by scheduler in idle task after no
  // user task ready for execute.
  paraos::freertos_idle_fnc_ptr = ExitAfterTestComplete;
#endif

  // ---------------------------------------------------------------------------
  // Create producers
  // ---------------------------------------------------------------------------
  Producer prod_1{0, "Prod 1"};
  producer_thread_numb += 1;

  Producer prod_2{1, "Prod 2"};
  producer_thread_numb += 1;

  Producer prod_3{2, "Prod 3"};
  producer_thread_numb += 1;

  Producer prod_4{3, "Prod 4"};
  producer_thread_numb += 1;
  // ---------------------------------------------------------------------------

  // ---------------------------------------------------------------------------
  // Create consumers
  // ---------------------------------------------------------------------------
  Consumer cons_1{"--Cons 1"};
  consumer_thread_numb += 1;

  Consumer cons_2{"--Cons 2"};
  consumer_thread_numb += 1;
  // ---------------------------------------------------------------------------

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  AssertsForTestComplete();

  return 0;
}
