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

/// @brief Total bytes numb in str_array with null terminate symbols in each
/// string.
std::size_t total_bytes_for_write{0};

/// @brief Numb of written bytes by all producers. Update in runtime.
std::size_t total_written_bytes{0};

std::vector<std::string> read_array;

/// @brief Numb of read bytes by all consumers. Update in runtime.
std::size_t total_read_bytes{0};

constexpr std::size_t queue_size{2};
constexpr std::size_t ring_buff_size{2048};
constexpr std::size_t ring_buff_numb{4};

std::size_t thread_total_numb{0};
std::size_t thread_exit_cnt{0};

std::size_t write_idx{0};
std::size_t read_idx{0};

paraos::MultiRingBuff<
    queue_size, paraos::RingBuff<std::uint8_t, ring_buff_size>,
    paraos::RingBuff<std::uint8_t, ring_buff_size>,
    paraos::RingBuff<std::uint8_t, ring_buff_size>,
    paraos::RingBuff<std::uint8_t, ring_buff_size>,
    paraos::RingBuff<std::uint8_t, ring_buff_size>>
    multi_ring_buff{};

std::vector<std::string> split(const char *buf, std::size_t buff_size) {
  std::vector<std::string> drives;
  std::string tmp_str = "";
  for (std::size_t i = 0; i < buff_size; i++) {
    auto chr = buf[i];
    if (int(chr) == 0) {
      drives.push_back(tmp_str);
      tmp_str = "";
    }
    // Символ "[", которым заполняется весь массив перед записью строки. В
    // данном случае обозначает "незанятую" ячейку.
    else if (int(chr) == 91) {
      break;
    } else {
      tmp_str += chr;
    }
  }
  return drives;
}

struct Producer : public paraos::Thread {
  Producer(
      const std::size_t thread_id, const std::string name = "Producer",
      std::size_t stack_depth = 1024,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kIdle)
      : paraos::Thread{name, stack_depth, priority}, thread_id_{thread_id} {
    paraos::Thread::SetNeedWhile(true);
    Start();
  }

  void Run() override {
    std::size_t idx;
    constexpr std::size_t write_delay_ms{100};

    {
      const paraos::CriticalSection critical;
      idx = write_idx;
      ++write_idx;
    }

    if (idx < str_array.size()) {
      auto str = str_array[idx];

      const std::size_t strl_len_with_null = str.size() + 1u;
      auto written_bytes_numb = multi_ring_buff.Write(
          thread_id_, static_cast<const void *>(str.data()), strl_len_with_null,
          write_delay_ms);

      {
        const paraos::CriticalSection critical;
        total_written_bytes += written_bytes_numb;

        std::cout << "Producer with id " << thread_id_
                  << " writing string: " << str << std::endl;
      }

      PARAOS_CHECK_ASSERT(
          written_bytes_numb == strl_len_with_null &&
          "Don't write string in ring buffer");
      paraos::Thread::DelayMs(1);
    } else {
      paraos::Thread::SetNeedWhile(false);
      const paraos::CriticalSection critical;
      std::cout << "Producer with id " << thread_id_ << " Exiting ... "
                << std::endl;
      ++thread_exit_cnt;
    }
  }

 private:
  std::size_t thread_id_;
};

struct Consumer : public paraos::Thread {
  Consumer(
      const std::string name = "Consumer", std::size_t stack_depth = 1024,
      paraos::ThreadPriority priority = paraos::ThreadPriority::kIdle)
      : paraos::Thread{name, stack_depth, priority} {
    paraos::Thread::SetNeedWhile(true);
    Start();
  }

  void Run() override {
    std::size_t idx;
    constexpr std::size_t read_delay_ms{15u};
    constexpr std::size_t read_mem_size{2048};

    auto read_mem = std::make_unique<std::array<std::uint8_t, read_mem_size>>();
    read_mem->fill('[');
    auto read_bytes_numb = multi_ring_buff.Read(
        idx, static_cast<void *>(read_mem->data()), read_mem->size(),
        read_delay_ms);

    const paraos::CriticalSection critical;
    if (read_bytes_numb != 0) {
      auto str_container = split(
          reinterpret_cast<const char *>(read_mem->data()), read_mem_size);

      std::cout << "Consumer " << Name()
                << " got str container with size: " << str_container.size()
                << std::endl;
      for (auto &str : str_container) {
        std::cout << "Consumer " << Name() << " read string: " << str
                  << std::endl;
      }

      read_array.push_back(reinterpret_cast<const char *>(read_mem->data()));
      read_idx += str_container.size();
      total_read_bytes += read_bytes_numb;
    }

    if (!(read_idx < str_array.size())) {
      paraos::Thread::SetNeedWhile(false);
      ++thread_exit_cnt;
    }
  }
};

void AssertsForTestComplete() {
  assert(
      total_bytes_for_write == total_written_bytes &&
      "Producers not write all bytes");

  std::cout << "Expect read bytes: " << total_bytes_for_write << std::endl;
  std::cout << "Actual read bytes: " << total_read_bytes << std::endl;
  assert(
      total_bytes_for_write == total_read_bytes &&
      "Consumers not read all bytes");
}

#if defined(FREERTOS)
void ExitAfterTestComplete() {
  auto is_need_exit{false};
  {
    paraos::CriticalSection critical;
    if (thread_exit_cnt == thread_total_numb) {
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

  for (auto &str : str_array) {
    total_bytes_for_write += str.size() + 1u;
  }

  Producer prod_1{0, "Prod 1"};
  Producer prod_2{1, "Prod 2"};
  Producer prod_3{2, "Prod 3"};
  Producer prod_4{2, "Prod 4"};
  thread_total_numb += 4;

  Consumer cons_1{"Cons 1"};
  Consumer cons_2{"Cons 2"};
  Consumer cons_3{"Cons 3"};
  thread_total_numb += 3;

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  AssertsForTestComplete();

  return 0;
}
