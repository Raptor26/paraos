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

// NOLINTBEGIN(misc-include-cleaner, readability-magic-numbers)
#include <array>
#include <atomic>
#include <cstddef>
#include <iostream>
#include <memory>

#include "etl/cyclic_value.h"
#include "paraos_check.h"
#include "paraos_critical.hpp"
#include "paraos_multi_ringbuff.hpp"
#include "paraos_ringbuff.hpp"
#include "paraos_thread.hpp"

#define PrintDebug(__message__, __object_name__)                             \
  {                                                                          \
    const paraos::CriticalSection macro_critical;                            \
    std::cout << "DM: '" << __object_name__ << "': " << __message__ << "\n"; \
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

constexpr std::size_t queue_size{2};
constexpr std::size_t ring_buff_size{2048};
constexpr std::size_t thread_stack_depth{1024};

namespace {
paraos::Thread check_test_complete_and_exit{paraos::ThreadAttr{
    "Check test complete", paraos::GetStackMinimumSizeInBytes(),
    paraos::ThreadPriority::kLowest, nullptr}};

auto CalcTotalBytesInStringArray(const std::vector<std::string> &str_arr)
    -> std::size_t {
  std::size_t total_bytes_numb{0};

  for (const auto &str : str_arr) {
    total_bytes_numb += str.length();
  }
  return total_bytes_numb;
}

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
}  // namespace

// String copy here is needed because of the delayed thread initialization -
// address of it's name could be invalid later.
// NOLINTBEGIN(performance-unnecessary-value-param)
struct Producer {
  explicit Producer(const paraos::ThreadAttr &attr, std::size_t thread_id)
      : thread_{attr}, thread_id_{thread_id} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<Producer, &Producer::Run>(
            *this));
  }

  /// @brief Producer thread
  void Run() {
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
            " string write successful: " << str_array.at(str_idx).c_str(),
            thread_.GiveName());
        break;
      }
      PrintDebug(
          "WARN: Nothin written, try again after delay. " << "String idx is "
                                                          << str_idx,
          thread_.GiveName());

      // If no consumers online, nobody read data from buffer and buffer
      // always will full.
      if (IsConsumersOffline()) {
        ThreadExit();
        break;
      }

      // Small delay for yeld resources.
      paraos::Thread::DelayMs(1);
    }

    // cyclic increment buff idx.
    ++buff_idx_;
  }

 private:
  void ThreadExit() {
    ++producer_thread_exit_cnt;
    PrintDebug(" exiting ... ", thread_.GiveName());
    thread_.Finished();
  }

  static auto IsConsumersOffline() -> bool {
    bool is_need_exit{false};

    if (consumer_thread_exit_cnt >= consumer_thread_numb) {
      is_need_exit = true;
    }

    return is_need_exit;
  }

 private:
  const std::size_t thread_id_;
  etl::cyclic_value<int, 0, multi_ring_buff.GetBuffNumb() - 1U> buff_idx_{0};
  paraos::Thread thread_;
};

struct Consumer {
  explicit Consumer(const paraos::ThreadAttr &attr) : thread_{attr} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<Consumer, &Consumer::Run>(
            *this));
  }

  /// @brief Consumer thread.
  void Run() {
    // Small delay for yeld recourses if no data available in buff.
    constexpr std::size_t delay_ms{1};
    constexpr std::size_t read_mem_size{2048};
    std::size_t idx;
    auto read_mem = std::make_unique<std::array<char, read_mem_size>>();

    auto read_size =
        multi_ring_buff.Read(idx, read_mem->data(), read_mem->size(), delay_ms);

    if (read_size > 0U) {
      consumer_total_read_bytes += read_size;
      PrintDebug(" read " << read_mem->data(), thread_.GiveName());
    } else {
      PrintDebug(
          " Nothing read, try again. Already read total bytes is "
              << consumer_total_read_bytes,
          thread_.GiveName());
    }

    // No producers online, nobody write new data, need exit from thread.
    if (IsProducersOffline()) {
      ThreadExit();
    }
  }

 private:
  void ThreadExit() {
    ++consumer_thread_exit_cnt;
    PrintDebug(" exiting ... ", thread_.GiveName());
    thread_.Finished();
  }

  static auto IsProducersOffline() -> bool {
    bool is_offline{false};
    if (producer_thread_exit_cnt >= producer_thread_numb) {
      is_offline = true;
    }

    return is_offline;
  }

  paraos::Thread thread_;
};
// NOLINTEND(performance-unnecessary-value-param)

namespace {
void AssertsForTestComplete() {
  constexpr std::size_t read_mem_size{2048};
  std::size_t idx;

  auto read_mem = std::make_unique<std::array<char, read_mem_size>>();

  for (std::size_t i = 0U; i < multi_ring_buff.GetBuffNumb(); ++i) {
    auto read_bytes_numb =
        multi_ring_buff.TryRead(idx, read_mem->data(), read_mem->size());

    if (read_bytes_numb > 0) {
      consumer_total_read_bytes += read_bytes_numb;
      PrintDebug("Read some data befor after all threads works complete", "");
    }
  }

  PrintDebug(
      "Total read bytes numb is " << consumer_total_read_bytes
                                  << ". Expected bytes numb is "
                                  << CalcTotalBytesInStringArray(str_array),
      "AssertsForTestComplete()");

  PARAOS_CHECK_ASSERT(
      consumer_total_read_bytes == producer_total_written_bytes &&
      "Total read and written bytes not equal");

  PARAOS_CHECK_ASSERT(
      producer_total_written_bytes == CalcTotalBytesInStringArray(str_array) &&
      "written bytes not equal with expected");
}

void ExitFromTest() {
  if (((consumer_thread_exit_cnt >= consumer_thread_numb) &&
       (producer_thread_exit_cnt >= producer_thread_numb))) {
    check_test_complete_and_exit.Finished();

    constexpr std::size_t delay_ms{0};
    PrintDebug("Ready to exit, delay ms " << delay_ms, "ExitFromTest");
    paraos::Thread::DelayMs(delay_ms);

    AssertsForTestComplete();

    PrintDebug("Call paraos::Thread::Exit();", "ExitFromTest");
    paraos::Thread::Exit();
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
  // Create producers
  // ---------------------------------------------------------------------------
  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 0";
    const static Producer prod_1{attr, 0};
    producer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 1";
    const static Producer prod_2{attr, 1};
    producer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 2";
    const static Producer prod_3{attr, 2};
    producer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 3";
    const static Producer prod_4{attr, 3};
    producer_thread_numb += 1;
  }
  // ---------------------------------------------------------------------------

  // ---------------------------------------------------------------------------
  // Create consumers
  // ---------------------------------------------------------------------------
  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 1";
    const static Consumer cons_1{attr};
    consumer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 2";
    const static Consumer cons_2{attr};
    consumer_thread_numb += 1;
  }

  // ---------------------------------------------------------------------------

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  AssertsForTestComplete();

  return 0;
}
// NOLINTEND(misc-include-cleaner, readability-magic-numbers)
