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

#include "paraos_bool_atomic.hpp"
#include "paraos_critical.hpp"
#include "paraos_multi_ringbuff.hpp"
#include "paraos_thread.hpp"
#include "paraos_utils.hpp"

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

/// @brief After each successful write, producer update this value.
std::atomic_size_t producer_total_written_bytes{0};

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

  void Run() override {
    std::size_t str_idx = producer_actual_str_idx;
    ++producer_actual_str_idx;

    if (str_idx < str_array.size()) {
      while (true) {
        // All consumers offline, need break thread.
        if (consumer_thread_exit_cnt >= producer_thread_numb) {
          SetNeedWhile(false);
          break;
        }

        auto written_bytes_numb = multi_ring_buff.TryWrite(
            thread_id_, str_array.at(str_idx).data(),
            str_array.at(str_idx).length());

        if (written_bytes_numb > 0) {
          PARAOS_CHECK_ASSERT(
              written_bytes_numb == str_array.at(str_idx).length() &&
              "Written not all bytes in string");

          producer_total_written_bytes += written_bytes_numb;
          paraos::CriticalSection critical;
          std::cout << "Producer with id " << thread_id_
                    << " writing string: " << str_array.at(str_idx).c_str()
                    << std::endl;
          break;
        } else {
          // try write again after small delay.
          constexpr std::size_t delay_ms{20};
          DelayMs(thread_id_ + delay_ms);
        }
      }
    } else {
      SetNeedWhile(false);
      ++producer_thread_exit_cnt;
      std::cout << "Producer with id " << thread_id_ << " exiting ... "
                << "total written bytes " << producer_total_written_bytes
                << " expect written bytes "
                << CalcTotalBytesInStringArray(str_array) << std::endl;
    }
  }

 private:
  const std::size_t thread_id_;
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

  void Run() override {
    constexpr std::size_t read_delay_ms{10};
    constexpr std::size_t read_mem_size{2048};

    // All producers offline, no wait anymore.
    if (producer_thread_exit_cnt >= producer_thread_numb) {
      SetNeedWhile(false);
      ++consumer_thread_exit_cnt;
    } else {
      std::size_t idx;
      auto read_mem = std::make_unique<std::array<char, read_mem_size>>();

      auto read_bytes_numb = multi_ring_buff.Read(
          idx, read_mem->data(), read_mem->size(), read_delay_ms);

      if (read_bytes_numb > 0) {
        consumer_total_read_bytes += read_bytes_numb;
        paraos::CriticalSection critical;
        std::cout << "Consumer " << Name()
                  << " read string: " << read_mem->data() << std::endl;
      } else if (
          producer_total_written_bytes >=
          CalcTotalBytesInStringArray(str_array)) {
        // All bytes already written in buffer (by producers) and no data
        // available in timeout. Will try read anything from buffer befor start
        // checks in AssertsForTestComplete().
        SetNeedWhile(false);
        ++consumer_thread_exit_cnt;

        std::cout << Name() << " exiting ..." << std::endl;
      }
    }
  }
};

void AssertsForTestComplete() {
  constexpr std::size_t read_mem_size{2048};
  std::size_t idx;

  auto read_mem = std::make_unique<std::array<char, read_mem_size>>();

  for (std::size_t i = 0u; i < multi_ring_buff.GetBuffNumb(); ++i) {
    auto read_bytes_numb =
        multi_ring_buff.ForceRead(idx, read_mem->data(), read_mem->size());

    if (read_bytes_numb > 0) {
      consumer_total_read_bytes += read_bytes_numb;
      std::cout << "Read some data befor after all threads works complete"
                << std::endl;
    }
  }

  std::cout << "Total read bytes numb is " << consumer_total_read_bytes
            << std::endl;

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

  Producer prod_1{0, "Prod 1"};
  Producer prod_2{1, "Prod 2"};
  Producer prod_3{2, "Prod 3"};
  Producer prod_4{3, "Prod 4"};
  producer_thread_numb += 4;

  Consumer cons_1{"Cons 1"};
  consumer_thread_numb += 1;

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  AssertsForTestComplete();

  return 0;
}
