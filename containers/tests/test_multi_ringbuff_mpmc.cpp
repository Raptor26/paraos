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
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>

#include "etl/cyclic_value.h"
#include "paraos_check.h"
#include "paraos_critical.hpp"
#include "paraos_multi_ringbuff.hpp"
#include "paraos_ringbuff.hpp"
#include "paraos_thread.hpp"

#define PrintDebug(__message__, __object_name__)               \
  {                                                            \
    const paraos::CriticalSection macro_critical;              \
                                                               \
    const std::time_t result = std::time(nullptr);             \
                                                               \
    std::cout << "Time: '" << result << " " << __object_name__ \
              << "': " << __message__ << "\n";                 \
  }

const std::vector<std::string> str_array{
    "1) We're talking away", "2) I don't know what I'm to say",
    "3) I'll say it anyway", "4) Today is another day to find you",
    "5) Shyin' away",        "6) Oh, I'll be comin' for your love, okay",
    "7) Take on me",         "8) -------------------------------------------"};

constexpr std::size_t queue_size{2};
constexpr std::size_t ring_buff_size{2048};

namespace {
paraos::Thread check_test_complete_and_exit{paraos::ThreadAttr{
    "Check test complete", paraos::GetStackMinimumSizeInBytes(),
    paraos::ThreadPriority::kRealTime, nullptr}};

auto CalcTotalBytesInStringArray(const std::vector<std::string> &str_arr)
    -> std::size_t {
  std::size_t total_bytes_numb{0};

  for (const auto &str : str_arr) {
    total_bytes_numb += str.length();
  }
  return total_bytes_numb;
}

std::size_t container_bytes_numb{0};

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

struct Producer {
  explicit Producer(const paraos::ThreadAttr &attr, std::size_t str_idx)
      : thread_{attr}, str_idx_{str_idx} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<Producer, &Producer::Run>(*this));
  }

  /// @brief Producer thread
  void Run() {
    while (true) {
      const std::size_t buff_idx = str_idx_ % multi_ring_buff.GetBuffNumb();
      if (str_idx_ < str_array.size()) {
      } else {
        // All data already written, exit from thread.
        ThreadExit();
        break;
      }

      // try write data in buffer periodical.
      auto written_len = multi_ring_buff.TryWrite(
          buff_idx, str_array.at(str_idx_).c_str(),
          str_array.at(str_idx_).length());

      if (written_len > 0) {
        // Break trying write data in buff, in next iteration take new string
        // idx for write in buff.
        producer_total_written_bytes += written_len;
        PrintDebug(
            " string write successful: " << str_array.at(str_idx_).c_str(),
            thread_.GiveName());
        ThreadExit();
        break;
      }
      PrintDebug(
          "WARN: Nothin written, try again after delay. " << "String idx is "
                                                          << str_idx_,
          thread_.GiveName());

      // Small delay for yeld resources.
      paraos::Thread::DelayMs(1);
    }
  }

 private:
  void ThreadExit() {
    ++producer_thread_exit_cnt;
    PrintDebug(" exiting ... ", thread_.GiveName());
    thread_.Finished();
  }

 private:
  const std::size_t str_idx_;
  paraos::Thread thread_;
};

struct Consumer {
  explicit Consumer(const paraos::ThreadAttr &attr) : thread_{attr} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<Consumer, &Consumer::Run>(*this));
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
      ThreadExit();
    } else {
      PrintDebug(
          " Nothing read, try again. Already read total bytes is "
              << consumer_total_read_bytes,
          thread_.GiveName());

      if (consumer_total_read_bytes >= container_bytes_numb) {
        ThreadExit();
      }
    }
  }

 private:
  void ThreadExit() {
    ++consumer_thread_exit_cnt;
    PrintDebug(" exiting ... ", thread_.GiveName());
    thread_.Finished();
  }

  paraos::Thread thread_;
};

namespace {
void AssertsForTestComplete() {
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
  container_bytes_numb = CalcTotalBytesInStringArray(str_array);

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
    const static Producer prod_0{attr, 0};
    producer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 1";
    const static Producer prod_1{attr, 1};
    producer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 2";
    const static Producer prod_2{attr, 2};
    producer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 3";
    const static Producer prod_3{attr, 3};
    producer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 4";
    const static Producer prod_4{attr, 4};
    producer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 5";
    const static Producer prod_5{attr, 5};
    producer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 6";
    const static Producer prod_6{attr, 6};
    producer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "Prod 7";
    const static Producer prod_7{attr, 7};
    producer_thread_numb += 1;
  }
  // ---------------------------------------------------------------------------

  // ---------------------------------------------------------------------------
  // Create consumers
  // ---------------------------------------------------------------------------
  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 0";
    const static Consumer cons_0{attr};
    consumer_thread_numb += 1;
  }

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

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 3";
    const static Consumer cons_3{attr};
    consumer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 4";
    const static Consumer cons_4{attr};
    consumer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 5";
    const static Consumer cons_5{attr};
    consumer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 6";
    const static Consumer cons_6{attr};
    consumer_thread_numb += 1;
  }

  {
    paraos::ThreadAttr attr{};
    attr.thread_name = "--Cons 7";
    const static Consumer cons_7{attr};
    consumer_thread_numb += 1;
  }

  // ---------------------------------------------------------------------------

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  AssertsForTestComplete();

  return 0;
}
// NOLINTEND(misc-include-cleaner, readability-magic-numbers)
