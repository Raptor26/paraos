/// @file test_multi_ringbuff_mpmc.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
// NOLINTBEGIN(misc-include-cleaner, readability-magic-numbers)
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "etl/cyclic_value.h"
#include "paraos_check.h"
#include "paraos_critical.hpp"
#include "paraos_jthread.hpp"
#include "paraos_multi_ringbuff.hpp"
#include "paraos_ringbuff.hpp"
#include "paraos_sleep.hpp"
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

const std::vector<std::string> str_array{
    "1) We're talking away", "2) I don't know what I'm to say",
    "3) I'll say it anyway", "4) Today is another day to find you",
    "5) Shyin' away",        "6) Oh, I'll be comin' for your love, okay",
    "7) Take on me",         "8) -------------------------------------------"};

constexpr std::size_t queue_size{2};
constexpr std::size_t ring_buff_size{2048};

namespace {

auto CalcTotalBytesInStringArray(  // NOLINT(llvm-prefer-static-over-anonymous-namespace): using static triggers misc-use-anonymous-namespace; keep internal linkage via anonymous namespace.
const std::vector<std::string>& str_arr)
    -> std::size_t {
  std::size_t total_bytes_numb{0};

  for (const auto& str : str_arr) {
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

std::mutex g_done_mtx;
std::condition_variable g_done_cv;
bool g_scheduler_ended{false};

void NotifySchedulerEnded() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  const std::scoped_lock lock{g_done_mtx};
  g_scheduler_ended = true;
  g_done_cv.notify_one();
}

void WaitForSchedulerEnded() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  std::unique_lock lock{g_done_mtx};
  g_done_cv.wait(lock, []() -> bool { return g_scheduler_ended; });
}
/// ----------------------------------------------------------------------------

paraos::MultiRingBuff<
    queue_size, char, paraos::RingBuff<char, ring_buff_size>,
    paraos::RingBuff<char, ring_buff_size>,
    paraos::RingBuff<char, ring_buff_size>,
    paraos::RingBuff<char, ring_buff_size>,
    paraos::RingBuff<char, ring_buff_size>>
    multi_ring_buff{};

struct Producer {
  explicit Producer(std::string_view name, std::size_t str_idx)
      : name_{name}, str_idx_{str_idx} {}

  /// @brief Producer thread
  void operator()(const paraos::stop_token& token) {
    while (!token.stop_requested()) {
      const std::size_t buff_idx = str_idx_ % multi_ring_buff.GetBuffNumb();
      if (str_idx_ < str_array.size()) {
      } else {
        // All data already written, exit from thread.
        ThreadExit();
        break;
      }

      // try write data in buffer periodical.
      auto how_many_bytes_need_write = str_array.at(str_idx_).length();
      auto is_write_successful = multi_ring_buff.TryWrite(
          buff_idx, str_array.at(str_idx_).c_str(), how_many_bytes_need_write);

      if (is_write_successful) {
        // Break trying write data in buff, in next iteration take new string
        // idx for write in buff.
        producer_total_written_bytes += how_many_bytes_need_write;
        PrintDebug(
            " string write successful: " << str_array.at(str_idx_).c_str(),
            name_);
        ThreadExit();
        break;
      }
      PrintDebug(
          "WARN: Nothin written, try again after delay. " << "String idx is "
                                                          << str_idx_,
          name_);

      // Small delay for yeld resources.
      paraos::sleep_for(std::chrono::milliseconds(1));
    }
  }

 private:
  void ThreadExit() {
    ++producer_thread_exit_cnt;
    PrintDebug(" exiting ... ", name_);
  }

 private:
  std::string_view name_;
  const std::size_t str_idx_;
};

struct Consumer {
  explicit Consumer(std::string_view name) : name_{name} {}

  /// @brief Consumer thread.
  void operator()(const paraos::stop_token& token) {
    // Small delay for yeld recourses if no data available in buff.
    constexpr std::size_t delay_ms{2000};
    constexpr std::size_t read_mem_size{2048};
    std::size_t idx;
    auto read_mem = std::make_unique<std::array<char, read_mem_size>>();

    while (!token.stop_requested()) {
      auto read_size = multi_ring_buff.Read(
          idx, read_mem->data(), read_mem->size(), delay_ms);

      if (read_size > 0U) {
        consumer_total_read_bytes += read_size;
        PrintDebug(" read " << read_mem->data(), name_);
        ThreadExit();
        return;
      }
      PrintDebug(
          " Nothing read, try again. Already read total bytes is "
              << consumer_total_read_bytes,
          name_);

      if (consumer_total_read_bytes >= container_bytes_numb) {
        ThreadExit();
        return;
      }
    }
  }

 private:
  void ThreadExit() {
    ++consumer_thread_exit_cnt;
    PrintDebug(" exiting ... ", name_);
  }

  std::string_view name_;
};

void AssertsForTestComplete(  // NOLINT(llvm-prefer-static-over-anonymous-namespace): using static triggers misc-use-anonymous-namespace; keep internal linkage via anonymous namespace.
) {
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

void IdleHook() {
  WaitForSchedulerEnded();
  AssertsForTestComplete();
  (void)paraos::jthread::end_scheduler();
}

}  // namespace

auto main() -> int {
  container_bytes_numb = CalcTotalBytesInStringArray(str_array);
  producer_thread_numb = 8;
  consumer_thread_numb = 8;

  // ---------------------------------------------------------------------------
  // Create producers and consumers inside an inner scope so that jthread
  // destructors join before final assertions.
  // ---------------------------------------------------------------------------
  {
    std::vector<paraos::jthread> threads;

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "Prod 0";
      threads.emplace_back(attr, Producer{"Prod 0", 0});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "Prod 1";
      threads.emplace_back(attr, Producer{"Prod 1", 1});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "Prod 2";
      threads.emplace_back(attr, Producer{"Prod 2", 2});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "Prod 3";
      threads.emplace_back(attr, Producer{"Prod 3", 3});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "Prod 4";
      threads.emplace_back(attr, Producer{"Prod 4", 4});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "Prod 5";
      threads.emplace_back(attr, Producer{"Prod 5", 5});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "Prod 6";
      threads.emplace_back(attr, Producer{"Prod 6", 6});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "Prod 7";
      threads.emplace_back(attr, Producer{"Prod 7", 7});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Cons 0";
      threads.emplace_back(attr, Consumer{"--Cons 0"});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Cons 1";
      threads.emplace_back(attr, Consumer{"--Cons 1"});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Cons 2";
      threads.emplace_back(attr, Consumer{"--Cons 2"});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Cons 3";
      threads.emplace_back(attr, Consumer{"--Cons 3"});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Cons 4";
      threads.emplace_back(attr, Consumer{"--Cons 4"});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Cons 5";
      threads.emplace_back(attr, Consumer{"--Cons 5"});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Cons 6";
      threads.emplace_back(attr, Consumer{"--Cons 6"});
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Cons 7";
      threads.emplace_back(attr, Consumer{"--Cons 7"});
    }

    const paraos::jthread stopper(
        [](const paraos::stop_token& /*token*/) -> void {
          while (consumer_total_read_bytes.load() < container_bytes_numb) {
            paraos::sleep_for(std::chrono::milliseconds{10});
          }
          NotifySchedulerEnded();
        });

#if PARAOS_LIKE_FREERTOS
    paraos::freertos_idle_fnc_ptr = IdleHook;
#endif

    paraos::jthread::start_scheduler();

    // При использовании freeRTOS, мы никогда не попадем в строку ниже т.к. все
    // управление блокируется в paraos::jthread::start_scheduler();
    WaitForSchedulerEnded();
  }

  // В методе ниже проверяется что все данные считаны и вызывается
  // (void)paraos::jthread::end_scheduler(); Весь функционал завершения работы
  // потоков инкапсулирован в одном методе чтобы его можно было использовать в
  // paraos::freertos_idle_fnc_ptr при тестах freeRTOS. Это связано с тем, что
  // метод ниже никогда не будет вызван при использовании freeRTOS (из-за
  // перехвата управления при вызове paraos::jthread::start_scheduler(), поэтому
  // передается указатель на IdleHook, который периодически вызывается на
  // freERTOS)
  IdleHook();

  return 0;
}
// NOLINTEND(misc-include-cleaner, readability-magic-numbers)
