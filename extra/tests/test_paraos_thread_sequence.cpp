/// @file test_paraos_thread_sequence.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <mutex>

// We need to import "etl/delegate.h" to use delegate, but static analyzer
// can't see the delegate declaration there.
// NOLINTBEGIN(misc-include-cleaner, readability-magic-numbers)
#include "etl/delegate.h"
#include "etl/timer.h"
#include "paraos_check.h"
#include "paraos_sleep.hpp"
#include "paraos_thread_sequence.hpp"

#define PrintDebug(__message__, __object_name__)                             \
  {                                                                          \
    const paraos::CriticalSection macro_critical;                            \
    std::cout << "DM: '" << __object_name__ << "': " << __message__ << "\n"; \
  }

namespace {

constexpr uint32_t thread_sequence_call_period_us{1000U};

std::atomic<std::size_t> gyracc_call_cnt{0};
std::atomic<std::size_t> mag_call_cnt{0};
std::atomic<std::size_t> baro_call_cnt{0};

constexpr uint_least8_t max_task_in_sequence{3};
using ThreadSequenceTest = paraos::ThreadSequence<max_task_in_sequence>;
ThreadSequenceTest* thread_seq_ptr{nullptr};

constexpr uint32_t gyr_acc_max_call_cnt{4};

constexpr float mag_delegate_freq_hz{thread_sequence_call_period_us / 2.0F};
constexpr float baro_delegate_freq_hz{thread_sequence_call_period_us / 4.0F};

std::atomic<bool> is_test_complete{false};

std::mutex g_done_mtx;
std::condition_variable g_done_cv;
bool g_scheduler_ended{false};

template <typename T = float>
struct GyrAcc {
 public:
  GyrAcc() { std::cout << "GyrAcc Ctor" << "\n"; }

  ~GyrAcc() { std::cout << "~GyrAcc Dtor" << "\n"; }

  void Update() {
    std::cout << "GyrAcc Update()" << "\n";
    ++gyracc_call_cnt;

    assert(thread_seq_ptr);

    if (gyracc_call_cnt.load() < gyr_acc_max_call_cnt) {
      constexpr bool is_isr{false};
      thread_seq_ptr->NotifyGive(is_isr);
    } else {
      // Signal completion from the sequence thread; the stopper thread will
      // call Finish() because jthread::join() cannot be called from the
      // thread being joined.
      is_test_complete.store(true, std::memory_order_release);
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

void NotifySchedulerEnded() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  {
    const std::scoped_lock lock{g_done_mtx};
    g_scheduler_ended = true;
  }
  g_done_cv.notify_one();
}

void WaitForSchedulerEnded() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  std::unique_lock lock{g_done_mtx};
  g_done_cv.wait(lock, []() -> bool { return g_scheduler_ended; });
}

void IdleHook() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace)
  WaitForSchedulerEnded();

  PARAOS_CHECK_ASSERT(gyracc_call_cnt.load() == gyr_acc_max_call_cnt);
  // Frequency of the call mag is two times less than gyr_acc.
  PARAOS_CHECK_ASSERT(mag_call_cnt.load() == gyr_acc_max_call_cnt / 2);
  // Frequency of the call mag is four times less than gyr_acc.
  PARAOS_CHECK_ASSERT(baro_call_cnt.load() == gyr_acc_max_call_cnt / 4);

  (void)paraos::jthread::end_scheduler();
}

}  // namespace

auto main() -> int {
  {
    paraos::ThreadSequenceAttr attr{
        {{"Sequence thread", paraos::GetStackMinimumSizeInBytes(),
          paraos::ThreadPriority::kRealTime, nullptr}}};

    attr.period_in_us = thread_sequence_call_period_us;

    ThreadSequenceTest thread_sequence{attr};
    thread_seq_ptr = &thread_sequence;

    // We need to import "etl/delegate.h" to use delegate, but static analyzer
    // can't see the delegate declaration there.
    // NOLINTBEGIN(misc-include-cleaner)
    //  Compile time delegate. Delegate lifetime can't be less then lifetime
    // between Registered() and Unregistered() call methods.
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
        auto timer_id = thread_seq_ptr->Register(
            baro_delegate, baro_delegate_freq_hz, true);
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

    const paraos::jthread stopper(
        [](const paraos::stop_token& /*token*/) -> void {
          while (!is_test_complete.load(std::memory_order_acquire)) {
            paraos::sleep_for(std::chrono::milliseconds{10});
          }

          PARAOS_CHECK_ASSERT(thread_seq_ptr);
          constexpr bool is_dynamic{false};
          thread_seq_ptr->Finish(is_dynamic);

          NotifySchedulerEnded();
        });
    (void)stopper;

#if PARAOS_LIKE_FREERTOS
    paraos::freertos_idle_fnc_ptr = IdleHook;
#endif

    paraos::jthread::start_scheduler();

    WaitForSchedulerEnded();

    thread_seq_ptr = nullptr;
  }

  IdleHook();

  return 0;
}
// NOLINTEND(misc-include-cleaner, readability-magic-numbers)
