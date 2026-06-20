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

// We need to import "etl/delegate.h" to use delegate, but static analyzer
// can't see the delegate declaration there.
// NOLINTBEGIN(misc-include-cleaner, readability-magic-numbers)
#include "etl/atomic.h"
#include "etl/delegate.h"
#include "etl/timer.h"
#include "paraos_check.h"
#include "paraos_thread_sequence.hpp"

#define PrintDebug(__message__, __object_name__)                             \
  {                                                                          \
    const paraos::CriticalSection macro_critical;                            \
    std::cout << "DM: '" << __object_name__ << "': " << __message__ << "\n"; \
  }

namespace {

paraos::Thread check_test_complete_and_exit{paraos::ThreadAttr{
    "Check test complete", paraos::GetStackMinimumSizeInBytes(),
    paraos::ThreadPriority::kRealTime, nullptr}};

constexpr uint32_t thread_sequence_call_period_us{1000U};

std::size_t gyracc_call_cnt{0};
std::size_t mag_call_cnt{0};
std::size_t baro_call_cnt{0};

constexpr uint_least8_t max_task_in_sequence{3};
using ThreadSequenceTest = paraos::ThreadSequence<max_task_in_sequence>;
ThreadSequenceTest* thread_seq_ptr;

constexpr uint32_t gyr_acc_max_call_cnt{4};

constexpr float mag_delegate_freq_hz{thread_sequence_call_period_us / 2.0};
constexpr float baro_delegate_freq_hz{thread_sequence_call_period_us / 4.0};

etl::atomic_bool is_test_complete{false};

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
      constexpr bool is_isr{false};
      thread_seq_ptr->NotifyGive(is_isr);
    } else {
      // Stop test.
      constexpr bool is_dynamic{false};
      thread_seq_ptr->Finish(is_dynamic);
      is_test_complete = true;
    }
  }

  /// @brief Five rule.
  GyrAcc(GyrAcc&& other) = delete;
  auto operator=(GyrAcc&& other) -> GyrAcc& = delete;
  auto operator=(const GyrAcc& other) -> GyrAcc& = delete;
  GyrAcc(const GyrAcc& other) = delete;
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
  Mag(Mag&& other) = delete;
  auto operator=(Mag&& other) -> Mag& = delete;
  auto operator=(const Mag& other) -> Mag& = delete;
  Mag(const Mag& other) = delete;
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
  Baro(Baro&& other) = delete;
  auto operator=(Baro&& other) -> Baro& = delete;
  auto operator=(const Baro& other) -> Baro& = delete;
  Baro(const Baro& other) = delete;
};

GyrAccFloat gyr_acc;

void ExitFromTest(  // NOLINT(llvm-prefer-static-over-anonymous-namespace): using static triggers misc-use-anonymous-namespace; keep internal linkage via anonymous namespace.
) {
  if (is_test_complete) {
    check_test_complete_and_exit.Finished();

    PARAOS_CHECK_ASSERT(thread_seq_ptr);
    constexpr bool is_dynamic{false};
    thread_seq_ptr->Finish(is_dynamic);

    constexpr std::size_t delay_ms{0};
    PrintDebug("Ready to exit, delay ms " << delay_ms, "ExitFromTest");
    paraos::Thread::DelayMs(delay_ms);

    PrintDebug("Call paraos::Thread::Exit();", "ExitFromTest");
#ifdef PARAOS_LIKE_FREERTOS
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
  {
    static auto delegate = etl::delegate<void()>::create<ExitFromTest>();
    check_test_complete_and_exit.RegisterDelegate(delegate);
  }

  {
    paraos::ThreadSequenceAttr attr{
        {{"Sequence thread", paraos::GetStackMinimumSizeInBytes(),
          paraos::ThreadPriority::kRealTime, nullptr}}};

    attr.period_in_us = thread_sequence_call_period_us;

    static ThreadSequenceTest thread_sequence{attr};
    thread_seq_ptr = &thread_sequence;
  }

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
    auto timer_id = thread_seq_ptr->Register(gyr_acc_delegate, 0.0, true);
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
          thread_seq_ptr->Register(mag_delegate, mag_delegate_freq_hz, true);
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
          thread_seq_ptr->Register(baro_delegate, baro_delegate_freq_hz, true);
      assert(timer_id != etl::timer::id::NO_TIMER);
    }

    // mag_delegate will be destroyed here. After registered delegate in
    // sequence, no more need delegate for runtime mag::Update.
  }

  {
    // No space for register second delegate.
    auto timer_id = thread_seq_ptr->Register(gyr_acc_delegate, 0.0, true);
    assert(timer_id == etl::timer::id::NO_TIMER);
  }

  constexpr bool is_isr{false};
  thread_seq_ptr->NotifyGive(is_isr);

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  PARAOS_CHECK_ASSERT(gyracc_call_cnt == gyr_acc_max_call_cnt);

  // Frequency of the call mag is two times less than gyr_acc.
  PARAOS_CHECK_ASSERT(mag_call_cnt == gyr_acc_max_call_cnt / 2);

  // Frequency of the call mag is four times less than gyr_acc.
  PARAOS_CHECK_ASSERT(baro_call_cnt == gyr_acc_max_call_cnt / 4);

  // [clang-analyzer-core.StackAddressEscape]: Address of stack memory
  // associated with local variable 'thread_sequence' is still referred to by
  // the global variable 'thread_seq_ptr' upon returning to the caller.  This
  // will be a dangling reference
  thread_seq_ptr = nullptr;

  return 0;
}
// NOLINTEND(misc-include-cleaner, readability-magic-numbers)
