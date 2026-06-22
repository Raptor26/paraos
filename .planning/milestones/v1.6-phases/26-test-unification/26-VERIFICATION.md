---
phase: 26
status: passed
completed: 2026-06-22
---

# Phase 26 Verification: Test unification

## Success Criteria Check

1. ✅ `port_tests/test_jthread_basic.cpp` не содержит `std::_Exit()` и единообразен для PC и FreeRTOS.
2. ✅ Обновлённые тесты используют `paraos::sleep_for(std::chrono::milliseconds{...})`.
3. ✅ Контейнерные multithread-тесты вызывают `paraos::jthread::start_scheduler()` и не используют `std::_Exit()`.
4. ✅ `test_jthread_basic` проходит на PC.

## Verification Evidence

- PC tests run successfully (exit code 0):
  - `test_jthread_basic` — `OK`
  - `test_queue_blocking_mpsc`
  - `test_queue_blocking_spmc`
  - `test_queue_blocking_mpmc`
  - `test_multi_ringbuff_mpmc`
  - `test_message_multithread_many_producer_many_consumers`
- FreeRTOS builds successful for all six test targets under `freertos_debug_clang` and `freertos_debug_gcc`.
- Log: `.planning/phases/26-test-unification/26-07-verification.log`.

## Notes

- PC `end_scheduler()` skips self-join so the stopper thread can safely shut down the scheduler.
- Runtime execution of FreeRTOS tests remains host-limited; build verification is accepted on macOS POSIX simulator.
