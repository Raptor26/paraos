/// @file test_paraos_work_queue_multithread.cpp
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

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "paraos_semaphore.hpp"
#include "paraos_trace.hpp"
#include "paraos_work_queue.hpp"

using namespace paraos;
using namespace std;

constexpr std::size_t DEFAULT_WORK_QUEUE_STACK_SIZE{1024 * 2};

/// @brief I'll Meet You At Midnight
std::vector<std::string> str_src{
    "1) A summer evening on Les Champs",
    "2) A secret rendezvous they planned for days",
    "3) A sea of faces in a crowded cafe",
    "4) A sound of laughter as the music plays",
    "5) Jean-Claude was a student at the University",
    "6) Louise-Marie was just a world away",
    "7) He recalled the night they met was warm with laughter",
    "8) The words were music as she turned away"};

std::vector<std::string> str_dst;

/// @brief Source string cnt. WorkItem increment this cnt when put string in
/// str_dst container.
std::size_t src_str_cnt;

/// @brief This semaphore given by queue_work thread after all expected strings
/// will be read from "str_src".
SemaphoreBinary all_works_complete_sem;

volatile int WorkItemCount = 0;

std::unique_ptr<WorkQueue> wq_low_ptr{nullptr};
std::unique_ptr<WorkQueue> wq_high_ptr{nullptr};

class MyWorkItem final : public WorkItem {
 public:
  MyWorkItem(std::string str) : str_{str} {}

  ~MyWorkItem() {}

  virtual void Run() override {
    const paraos::CriticalSection critical;
    str_dst.push_back(str_);
    ++src_str_cnt;

    if (src_str_cnt >= str_src.size()) {
      all_works_complete_sem.Give();
    }
  }

 private:
  std::string str_;
};

class TestThread final : public Thread {
 public:
  TestThread(int i, std::size_t delayInSeconds)
      : Thread("TestThread", 100, ThreadPriority::kIdle),
        id(i),
        DelayInSeconds(delayInSeconds) {
    Start();
  }

 protected:
  void Run() {
    constexpr std::size_t queue_len{2};

    // High priority work queue.
    wq_high_ptr = std::make_unique<WorkQueue>(
        "wq_high", DEFAULT_WORK_QUEUE_STACK_SIZE, ThreadPriority::kHighest,
        queue_len);

    constexpr std::size_t delay_ms{10000};

    for (std::size_t i = 0, str_num = str_src.size() / 2; i < str_num; ++i) {
      wq_high_ptr->Push(std::make_unique<MyWorkItem>(str_src.at(i)), delay_ms);
      wq_low_ptr->Push(
          std::make_unique<MyWorkItem>(str_src.at(i + 1)), delay_ms);
    }

    all_works_complete_sem.Take(delay_ms);
  };

 private:
  int id;
  std::size_t DelayInSeconds;
};

/// @brief Multithread test for work queue.
///
/// @note Valgrind write "Potential Memory Leak - 2". This is due to the
/// implementation of pthread because glibc doesn't free thread stacks when
/// threads exit; it caches them for reuse, and only prunes the cache when it
/// gets huge. Thus it always "leaks" some memory.
/// @see https://stackoverflow.com/questions/7278216/pthread-create-memory-leak
///
/// @return 0 if test success.
int main() {
  TestThread thread(1, 1);

  constexpr std::size_t queue_len{1};

  // low priority work queue.
  wq_low_ptr = std::make_unique<WorkQueue>(
      "wq_high", DEFAULT_WORK_QUEUE_STACK_SIZE, ThreadPriority::kLowest,
      queue_len);

  // Code below check memory leak
  auto work_queue_one = std::make_unique<WorkQueue>(
      "test work_queue", DEFAULT_WORK_QUEUE_STACK_SIZE, ThreadPriority::kLowest,
      queue_len, false);

  // Code below check memory leak
  auto work_queue_two = WorkQueue(
      "test work_queue next", DEFAULT_WORK_QUEUE_STACK_SIZE,
      ThreadPriority::kLowest, queue_len, true);

  Thread::StartScheduler();
  Thread::DeleteAll();

  // All strings from 'str_src' must be pushed in 'str_dst by' work_queue.
  {
    assert(
        (str_src.size() == str_dst.size()) &&
        "Work item don't write all strings");

    for (auto &str : str_dst) {
      assert(
          std::find(str_src.begin(), str_src.end(), str) != str_src.end() &&
          "Can't find consumer string in source container");
    }
  }

  // unique_ptr free resources automaticaly, but we forced call Dtor, contained
  // in unique_ptr.
  wq_high_ptr.reset(nullptr);

// unique_ptr free resources automaticaly. Code below only for debug.
#if 0
    wq_low_ptr.reset(nullptr);
#endif
  return 0;
}