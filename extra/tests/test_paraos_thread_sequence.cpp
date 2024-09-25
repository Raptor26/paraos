/// @file test_paraos_thread_sequence_v2.cpp
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
#include <string>

#include "etl/delegate.h"
#include "etl/function.h"
#include "etl/scheduler.h"
#include "etl/task.h"
#include "paraos_thread_sequence.hpp"
#include "paraos_utils.hpp"

namespace {

std::size_t gyracc_call_cnt{0};
std::size_t mag_call_cnt{0};
std::size_t baro_call_cnt{0};

constexpr uint_least8_t max_task_in_sequence{3};
using ThreadSequenceTest = paraos::ThreadSequence<max_task_in_sequence>;
ThreadSequenceTest* thread_seq_ptr;

constexpr uint32_t gyr_acc_max_call_cnt{8};

bool is_test_complete{false};

template <typename T = float>
struct GyrAcc {
 public:
  GyrAcc() { std::cout << "GyrAcc Ctor" << std::endl; }

  ~GyrAcc() { std::cout << "~GyrAcc Dtor" << std::endl; }

  void Update() {
    std::cout << "GyrAcc Update()" << std::endl;
    ++gyracc_call_cnt;

    assert(thread_seq_ptr);

    if (gyracc_call_cnt < gyr_acc_max_call_cnt) {
      thread_seq_ptr->NotifyGive();
    } else {
      // Stop test.
      thread_seq_ptr->Break();
      is_test_complete = true;
    }
  }
};

using GyrAccFloat = GyrAcc<float>;

struct Mag {
 public:
  Mag() { std::cout << "Mag Ctor" << std::endl; }

  ~Mag() { std::cout << "~Mag Dtor" << std::endl; }

  void Update() {
    ++mag_call_cnt;
    std::cout << "Mag Update()" << std::endl;
  }
};

struct Baro {
 public:
  Baro() { std::cout << "Baro Ctor" << std::endl; }

  ~Baro() { std::cout << "~Baro Dtor" << std::endl; }

  void Update() {
    ++baro_call_cnt;
    std::cout << "Baro Update()" << std::endl;
  }
};

GyrAccFloat gyr_acc;

/// FreeRTOS can't stop scheduler. In this case we must manually call
/// exit(EXIT_SUCCESS) after test complete.
#if defined(FREERTOS)
void ExitAfterTestComplete() {
  if (is_test_complete) {
    exit(EXIT_SUCCESS);
  }
}
#endif

}  // namespace

int main() {
#if defined(FREERTOS)
  // ExitAfterTestComplete will be called by scheduler in idle task after no
  // user task ready for execute.
  paraos::freertos_idle_fnc_ptr = ExitAfterTestComplete;
#endif

  using namespace paraos;

  constexpr uint32_t thread_sequence_call_period_us{1000u};
  ThreadSequenceTest thread_sequence{
      "Sequence static", 1024, ThreadPriority::kNormal,
      thread_sequence_call_period_us};

  thread_seq_ptr = &thread_sequence;

  // Compile time delegate. Delegate lifetime can't be less then lifetime
  // between Registered() and Unregistered() call methods.
  static auto gyr_acc_delegate = etl::delegate<void(
      void)>::create<GyrAccFloat, gyr_acc, &GyrAccFloat::Update>();

  {
    // gyr_acc_delegate will be run on each call NotifyGive(). In this case
    // frequency set 0.0.
    auto timer_id = thread_sequence.Register(gyr_acc_delegate, 0.0, true);
    assert(timer_id != etl::timer::id::NO_TIMER);
  }

  {
    // runtime variable.
    static Mag mag;

    // runtime delegate. Delegate lifetime can't be less then lifetime between
    // Registered() and Unregistered() call methods.
    static etl::delegate<void(void)> mag_delegate =
        etl::delegate<void(void)>::create<Mag, &Mag::Update>(mag);

    {
      // mag_delegate call period is two times less than gyr_acc_delegate
      auto timer_id = thread_sequence.Register(
          mag_delegate, thread_sequence_call_period_us / 2.0, true);
      assert(timer_id != etl::timer::id::NO_TIMER);
    }
  }

  {
    // runtime variable.
    static Baro baro;

    // runtime delegate. Delegate lifetime can't be less then lifetime between
    // Registered() and Unregistered() call methods.
    etl::delegate<void(void)> baro_delegate =
        etl::delegate<void(void)>::create<Baro, &Baro::Update>(baro);

    {
      // mag_delegate call period is four times less than gyr_acc_delegate
      auto timer_id = thread_sequence.Register(
          baro_delegate, thread_sequence_call_period_us / 4.0, true);
      assert(timer_id != etl::timer::id::NO_TIMER);
    }

    // mag_delegate will be destroyed here. After registered delegate in
    // sequence, no more need delegate for runtime mag::Update.
  }

  {
    // No space for register second delegate.
    auto timer_id = thread_sequence.Register(gyr_acc_delegate, 0.0, true);
    assert(timer_id == etl::timer::id::NO_TIMER);
  }

  thread_sequence.NotifyGive();

  Thread::StartScheduler();
  Thread::DeleteAll();

  PARAOS_CHECK_ASSERT(gyracc_call_cnt == gyr_acc_max_call_cnt);

  // Frequency of the call mag is two times less than gyr_acc.
  PARAOS_CHECK_ASSERT(mag_call_cnt == gyracc_call_cnt / 2);

  // Frequency of the call mag is four times less than gyr_acc.
  PARAOS_CHECK_ASSERT(baro_call_cnt == gyracc_call_cnt / 4);

  return 0;
}
