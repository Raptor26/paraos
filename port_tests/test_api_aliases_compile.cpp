/// @file test_api_aliases_compile.cpp
/// @brief Smoke test: deprecated aliases still compile.

#include <cstdint>

#include "paraos_base.hpp"
#include "paraos_bool_atomic.hpp"
#include "paraos_critical.hpp"
#include "paraos_isr.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_testing_semaphore.hpp"
#include "paraos_thread_common.hpp"
#include "paraos_timer.hpp"

#include "extra/paraos_status_led.hpp"
#include "containers/paraos_message_buffer.hpp"
#include "containers/paraos_queue_blocking.hpp"
#include "containers/paraos_ringbuff.hpp"

// Type aliases compile test
static_assert(sizeof(paraos::Base) == sizeof(paraos::base));
static_assert(sizeof(paraos::ISRbool) == sizeof(paraos::isr_bool));
static_assert(sizeof(paraos::ThreadAttr) == sizeof(paraos::thread_attr));

static_assert(sizeof(paraos::VarAtomic<int>) == sizeof(paraos::var_atomic<int>));
static_assert(sizeof(paraos::BoolAtomic) == sizeof(paraos::bool_atomic));

static_assert(sizeof(paraos::CriticalSection<>) == sizeof(paraos::critical_section<>));
static_assert(sizeof(paraos::Timer) == sizeof(paraos::timer));

static_assert(sizeof(paraos::IProfiler) == sizeof(paraos::profiler_base));
static_assert(sizeof(paraos::EmptyProfiler) == sizeof(paraos::empty_profiler));
static_assert(sizeof(paraos::EmbeddedTimerEmpty) ==
              sizeof(paraos::embedded_timer_empty));

static_assert(sizeof(paraos::IStatusLed) == sizeof(paraos::status_led_base));
static_assert(sizeof(paraos::IQueueBlocking<int, 8>) ==
              sizeof(paraos::queue_blocking_base<int, 8>));
static_assert(sizeof(paraos::IRingBuff<std::uint8_t>) ==
              sizeof(paraos::ring_buff_base<std::uint8_t>));
static_assert(sizeof(paraos::IMessageBuffer<>) ==
              sizeof(paraos::message_buffer_base<>));

// Old enum-value aliases compile test
static_assert(paraos::ThreadPriority::kIdle == paraos::thread_priority::idle);
static_assert(paraos::ThreadPriority::kLowest == paraos::thread_priority::lowest);
static_assert(paraos::ThreadPriority::kBelowNormal ==
              paraos::thread_priority::below_normal);
static_assert(paraos::ThreadPriority::kNormal == paraos::thread_priority::normal);
static_assert(paraos::ThreadPriority::kAboveNormal ==
              paraos::thread_priority::above_normal);
static_assert(paraos::ThreadPriority::kHighest == paraos::thread_priority::highest);
static_assert(paraos::ThreadPriority::kRealTime == paraos::thread_priority::realtime);

static_assert(paraos::StatusLedMode::kEnable == paraos::status_led_mode::enable);
static_assert(paraos::StatusLedMode::kDisable == paraos::status_led_mode::disable);

int main() {
  paraos::CriticalSection<>::ForceEnter();
  paraos::CriticalSection<>::ForceExit();

  // Deprecated method forwarding calls compile test
  paraos::testing_semaphore sem{paraos::testing_semaphore_attr{1U, 1U}};
  (void)sem.Take(0U, false);
  (void)sem.Give(false);

  paraos::ring_buff<std::uint8_t, 8> ring;
  (void)ring.Size();
  (void)ring.Capacity();
  (void)ring.IsEmpty();

  paraos::queue_blocking<int, 8> queue;
  (void)queue.Size();
  (void)queue.IsEmpty();

  paraos::message<> msg{1U};
  (void)msg.Size();
  msg.Free();

  // Reference to deprecated status_led forwarding method (avoids starting a
  // background thread while still verifying that the method compiles).
  auto new_blink_mode_ptr = &paraos::status_led::NewBlinkMode;
  (void)new_blink_mode_ptr;

  return 0;
}
