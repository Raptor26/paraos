/// @file test_message_multithread_many_producer_many_consumers.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
// NOLINTBEGIN(misc-include-cleaner, readability-magic-numbers)
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "paraos_config.hpp"
#include "paraos_critical.hpp"
#include "paraos_jthread.hpp"
#include "paraos_message_buffer.hpp"
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

/// @brief Burning Heart
const std::vector<std::string> elems_vector{
    "1)  Two worlds collide",     "2)  Rival nations",
    "3)  It's a primitive clash", "4)  Venting years of frustration",
    "5)  Bravely we hope",        "6)  Against all hope",
};

namespace {

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

struct Producer {
  explicit Producer(std::string_view name, std::size_t str_idx)
      : name_{name}, str_idx_{str_idx} {}

  /// @brief Producer thread.
  void operator()(const paraos::stop_token& token) {
    if (str_idx_ >= elems_vector.size()) {
      Exit();
      return;
    }

    // Trying to write message in buffer will not work yet.
    while (!token.stop_requested()) {
      auto write =
          message_buff.Alloc(elems_vector.at(str_idx_).length() + 1U);

      // If memory alloc successful.
      if (write) {
        memcpy(write.Data(), elems_vector.at(str_idx_).data(), write.Size());
        const auto is_push_success = write.TryPush();

        if (is_push_success) {
          PrintDebug(
              " string write successful: " << elems_vector.at(str_idx_).c_str(),
              name_);

          {
            const paraos::CriticalSection critical;
            producers_str_container.emplace_back(
                elems_vector.at(str_idx_).c_str());
          }

          Exit();
          return;
        }

        // Explicit TryPush() failed and the MessageWritable destructor would
        // retry the push automatically. Prevent that so a producer does not
        // insert a duplicate message while it is spinning waiting for a free
        // queue slot.
        write.Free();
      }
      PrintDebug(
          " WARN: Nothin written, try again after delay. " << "String idx is "
                                                           << str_idx_,
          name_);

      // Small delay for yeld resources.
      paraos::sleep_for(std::chrono::milliseconds(producer_waiting_timeout_ms));
    }
  }

 private:
  void Exit() {
    ++producer_thread_exit_cnt;
    PrintDebug(" exiting ... ", name_);
  }

  std::string_view name_;
  std::size_t str_idx_{1000};
};

struct Consumer {
  explicit Consumer(std::string_view name) : name_{name} {}

  /// @brief Consumers thread.
  void operator()(const paraos::stop_token& token) {
    while (!token.stop_requested()) {
      // Small delay for yeld resources.
      constexpr std::size_t delay_ms{2000};

      auto read_message = message_buff.Pop(delay_ms);

      if (read_message) {
        std::string read_str;
        {
          const paraos::CriticalSection critical;
          read_str = reinterpret_cast<char*>(read_message->Data());
        }

        {
          const paraos::CriticalSection critical;
          consumers_str_container.emplace_back(read_str);
        }

        PrintDebug(" string read successful: " << read_str, name_);

        read_message.reset();

        Exit();
        return;
      }
      paraos::sleep_for(std::chrono::milliseconds(consumer_waiting_timeout_ms));
    }
  }

 private:
  void Exit() {
    ++consumer_thread_exit_cnt;
    PrintDebug(" exiting ... ", name_);
  }

  std::string_view name_;
};

void CheckIfTestSuccessfullyComplete(  // NOLINT(llvm-prefer-static-over-anonymous-namespace): using static triggers misc-use-anonymous-namespace; keep internal linkage via anonymous namespace.
) {
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
  for (auto& str : consumers_str_container) {
    // Project targets C++17; std::ranges is unavailable.
    // NOLINTNEXTLINE(llvm-use-ranges)
    assert(
        std::find(elems_vector.begin(), elems_vector.end(), str) !=
            elems_vector.end() &&
        "Can't find consumer string in source container");
  }
}

void IdleHook() {
  WaitForSchedulerEnded();
  CheckIfTestSuccessfullyComplete();
  (void)paraos::jthread::end_scheduler();
}

}  // namespace

auto main() -> int {
  // ---------------------------------------------------------------------------
  // Create consumers and producers inside an inner scope so that jthread
  // destructors join before final assertions.
  // ---------------------------------------------------------------------------
  {
    std::vector<paraos::jthread> threads;

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Cons 0";
      attr.priority = paraos::ThreadPriority::kLowest;
      threads.emplace_back(attr, Consumer{"--Cons 0"});
      consumer_total_thread_numb += 1;
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Cons 1";
      attr.priority = paraos::ThreadPriority::kBelowNormal;
      threads.emplace_back(attr, Consumer{"--Cons 1"});
      consumer_total_thread_numb += 1;
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Cons 2";
      attr.priority = paraos::ThreadPriority::kNormal;
      threads.emplace_back(attr, Consumer{"--Cons 2"});
      consumer_total_thread_numb += 1;
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Cons 3";
      attr.priority = paraos::ThreadPriority::kNormal;
      threads.emplace_back(attr, Consumer{"--Cons 3"});
      consumer_total_thread_numb += 1;
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Cons 4";
      attr.priority = paraos::ThreadPriority::kNormal;
      threads.emplace_back(attr, Consumer{"--Cons 4"});
      consumer_total_thread_numb += 1;
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "--Cons 5";
      attr.priority = paraos::ThreadPriority::kLowest;
      threads.emplace_back(attr, Consumer{"--Cons 5"});
      consumer_total_thread_numb += 1;
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "Prod 0";
      attr.priority = paraos::ThreadPriority::kAboveNormal;
      threads.emplace_back(attr, Producer{"Prod 0", 0});
      producer_total_thread_numb += 1;
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "Prod 1";
      attr.priority = paraos::ThreadPriority::kHighest;
      threads.emplace_back(attr, Producer{"Prod 1", 1});
      producer_total_thread_numb += 1;
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "Prod 2";
      threads.emplace_back(attr, Producer{"Prod 2", 2});
      producer_total_thread_numb += 1;
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "Prod 3";
      threads.emplace_back(attr, Producer{"Prod 3", 3});
      producer_total_thread_numb += 1;
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "Prod 4";
      threads.emplace_back(attr, Producer{"Prod 4", 4});
      producer_total_thread_numb += 1;
    }

    {
      paraos::ThreadAttr attr{};
      attr.thread_name = "Prod 5";
      attr.priority = paraos::ThreadPriority::kAboveNormal;
      threads.emplace_back(attr, Producer{"Prod 5", 5});
      producer_total_thread_numb += 1;
    }

    const paraos::jthread stopper(
        [](const paraos::stop_token& /*token*/) -> void {
          while (consumer_thread_exit_cnt.load() <
                 consumer_total_thread_numb.load()) {
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
