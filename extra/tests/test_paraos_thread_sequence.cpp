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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

// We need to import "etl/delegate.h" to use delegate, but static analyzer
// can't see the delegate declaration there.
// NOLINTBEGIN(misc-include-cleaner)
#include "etl/delegate.h"
// NOLINTEND(misc-include-cleaner)

#include "etl/timer.h"
#include "paraos_check.h"
#include "paraos_thread.hpp"
#include "paraos_thread_sequence.hpp"

namespace {

constexpr uint32_t thread_sequence_call_period_us{1000U};

std::size_t gyracc_call_cnt{0};
std::size_t mag_call_cnt{0};
std::size_t baro_call_cnt{0};

constexpr size_t thread_default_stack_size{1024};

constexpr uint_least8_t max_task_in_sequence{3};
using ThreadSequenceTest = paraos::ThreadSequence<max_task_in_sequence>;
ThreadSequenceTest *thread_seq_ptr;

constexpr uint32_t gyr_acc_max_call_cnt{8};

constexpr float mag_delegate_freq_hz{thread_sequence_call_period_us / 2.0};
constexpr float baro_delegate_freq_hz{thread_sequence_call_period_us / 4.0};

bool is_test_complete{false};

template <typename T = float>
struct GyrAcc {
 public:
  GyrAcc() { std::cout << "GyrAcc Ctor" << "\n"; }

  ~GyrAcc() { std::cout << "~GyrAcc Dtor" << "\n"; }

  void Update() {
    std::cout << "GyrAcc Update()" << "\n";
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

  /// @brief Five rule.
  GyrAcc(GyrAcc &&other) = delete;
  auto operator=(GyrAcc &&other) -> GyrAcc & = delete;
  auto operator=(const GyrAcc &other) -> GyrAcc & = delete;
  GyrAcc(const GyrAcc &other) = delete;
};

using GyrAccFloat = GyrAcc<float>;

struct Mag {
 public:
  Mag() { std::cout << "Mag Ctor" << "\n"; }

  ~Mag() { std::cout << "~Mag Dtor" << "\n"; }

  // We can't make Update() method static, because it needs to be classmethod to
  // create etl::delegate.
  // NOLINTBEGIN(readability-convert-member-functions-to-static)
  void Update() {
    ++mag_call_cnt;
    std::cout << "Mag Update()" << "\n";
  }
  // NOLINTEND(readability-convert-member-functions-to-static)

  /// @brief Five rule.
  Mag(Mag &&other) = delete;
  auto operator=(Mag &&other) -> Mag & = delete;
  auto operator=(const Mag &other) -> Mag & = delete;
  Mag(const Mag &other) = delete;
};

struct Baro {
 public:
  Baro() { std::cout << "Baro Ctor" << "\n"; }

  ~Baro() { std::cout << "~Baro Dtor" << "\n"; }

  // We can't make Update() method static, because it needs to be classmethod to
  // create etl::delegate.
  // NOLINTBEGIN(readability-convert-member-functions-to-static)
  void Update() {
    ++baro_call_cnt;
    std::cout << "Baro Update()" << "\n";
  }
  // NOLINTEND(readability-convert-member-functions-to-static)

  /// @brief Five rule.
  Baro(Baro &&other) = delete;
  auto operator=(Baro &&other) -> Baro & = delete;
  auto operator=(const Baro &other) -> Baro & = delete;
  Baro(const Baro &other) = delete;
};

GyrAccFloat gyr_acc;

/// FreeRTOS can't stop scheduler. In this case we must manually call
/// exit(EXIT_SUCCESS) after test complete.
#if defined(FREERTOS)
#include <cstdlib>
void ExitAfterTestComplete() {
  if (is_test_complete) {
    exit(EXIT_SUCCESS);
  }
}
#endif

}  // namespace

auto main() -> int {
#if defined(FREERTOS)
#include "paraos_utils.hpp"

  // ExitAfterTestComplete will be called by scheduler in idle task after no
  // user task ready for execute.
  paraos::freertos_idle_fnc_ptr = ExitAfterTestComplete;
#endif
  ThreadSequenceTest thread_sequence{
      "Sequence static", thread_default_stack_size,
      paraos::ThreadPriority::kNormal, thread_sequence_call_period_us};

  thread_seq_ptr = &thread_sequence;

  // We need to import "etl/delegate.h" to use delegate, but static analyzer
  // can't see the delegate declaration there.
  // NOLINTBEGIN(misc-include-cleaner)
  //  Compile time delegate. Delegate lifetime can't be less then lifetime
  //  between Registered() and Unregistered() call methods.
  static auto gyr_acc_delegate = etl::delegate<void(
      void)>::create<GyrAccFloat, gyr_acc, &GyrAccFloat::Update>();
  // NOLINTEND(misc-include-cleaner)

  {
    // gyr_acc_delegate will be run on each call NotifyGive(). In this case
    // frequency set 0.0.
    auto timer_id = thread_sequence.Register(gyr_acc_delegate, 0.0, true);
    assert(timer_id != etl::timer::id::NO_TIMER);
  }

  {
    // runtime variable.
    static Mag mag;

    // We need to import "etl/delegate.h" to use delegate, but static analyzer
    // can't see the delegate declaration there.
    // NOLINTBEGIN(misc-include-cleaner)
    // runtime delegate. Delegate lifetime can't be less then lifetime between
    // Registered() and Unregistered() call methods.
    static etl::delegate<void(void)> mag_delegate =
        etl::delegate<void(void)>::create<Mag, &Mag::Update>(mag);
    // NOLINTEND(misc-include-cleaner)

    {
      // mag_delegate call period is two times less than gyr_acc_delegate
      auto timer_id =
          thread_sequence.Register(mag_delegate, mag_delegate_freq_hz, true);
      assert(timer_id != etl::timer::id::NO_TIMER);
    }
  }

  {
    // runtime variable.
    static Baro baro;

    // We need to import "etl/delegate.h" to use delegate, but static analyzer
    // can't see the delegate declaration there.
    // NOLINTBEGIN(misc-include-cleaner)
    // runtime delegate. Delegate lifetime can't be less then lifetime between
    // Registered() and Unregistered() call methods.
    etl::delegate<void(void)> baro_delegate =
        etl::delegate<void(void)>::create<Baro, &Baro::Update>(baro);
    // NOLINTEND(misc-include-cleaner)

    {
      // mag_delegate call period is four times less than gyr_acc_delegate
      auto timer_id =
          thread_sequence.Register(baro_delegate, baro_delegate_freq_hz, true);
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

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  PARAOS_CHECK_ASSERT(gyracc_call_cnt == gyr_acc_max_call_cnt);

  // Frequency of the call mag is two times less than gyr_acc.
  PARAOS_CHECK_ASSERT(mag_call_cnt == gyracc_call_cnt / 2);

  // Frequency of the call mag is four times less than gyr_acc.
  PARAOS_CHECK_ASSERT(baro_call_cnt == gyracc_call_cnt / 4);

  // [clang-analyzer-core.StackAddressEscape]: Address of stack memory
  // associated with local variable 'thread_sequence' is still referred to by
  // the global variable 'thread_seq_ptr' upon returning to the caller.  This
  // will be a dangling reference
  thread_seq_ptr = nullptr;

  return 0;
}
