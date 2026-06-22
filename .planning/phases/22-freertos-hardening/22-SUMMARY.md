# Phase 22: FreeRTOS std-like primitives hardening - Summary

**Phase:** 22
**Milestone:** v1.5 Modernize container tests on std-like primitives
**Status:** Complete
**Completed:** 2026-06-22

## Goal

Add the minimum FreeRTOS-specific hardening needed for the migrated container tests to behave uniformly across PC and FreeRTOS. The primary deliverable was a cross-platform `paraos::sleep_for` helper that replaced duplicated local `#ifdef` helpers in the migrated tests.

## What Changed

- Added public header `paraos_sleep.hpp` (already committed earlier in the phase) providing `paraos::sleep_for(std::chrono::milliseconds)`.
  - PC/Unix/Windows: delegates to `std::this_thread::sleep_for`.
  - FreeRTOS: delegates to `vTaskDelay(PARAOS_ConvertMsToTicks(...))`.
- Replaced local `SleepMs()` helpers with `paraos::sleep_for(std::chrono::milliseconds(...))` in five migrated multithreaded container tests:
  - `containers/tests/test_queue_blocking_spmc.cpp`
  - `containers/tests/test_queue_blocking_mpsc.cpp`
  - `containers/tests/test_queue_blocking_mpmc.cpp`
  - `containers/tests/test_multi_ringbuff_mpmc.cpp`
  - `containers/tests/test_message_multithread_many_producer_many_consumers.cpp`
- Fixed a race-induced hang in `test_message_multithread_many_producer_many_consumers.cpp`:
  - `MessageWritable::~MessageWritable()` auto-pushes the held message on destruction.
  - When `TryPush()` failed, the producer loop spun while the destructor could still push a duplicate message behind the producer's back, eventually leaving producers blocked after all consumers had exited.
  - Added `write.Free()` after a failed explicit `TryPush()` so the destructor has nothing to auto-push.

## FreeRTOS jthread Join Signaling

Inspected `port_freertos/paraos_jthread.hpp`:

- `RunTask` invokes the user callable, then gives `join_sem`, then calls `vTaskDelete(nullptr)`.
- The destructor-issued `join()` waits on `join_sem`; therefore a completed task cannot deadlock `join()`.
- No code change was required.

## Verification

- `pc_debug_clang` and `pc_debug_gcc` builds succeed.
- All 58 tests pass on PC Clang and PC GCC.
- Stress tests (`ctest -L stress --repeat-until-fail 100`) pass on both PC Clang and PC GCC.
- `freertos_debug_clang` and `freertos_debug_gcc` compile the five migrated tests without new errors.
- `paraos_sleep.hpp` compiles on all four presets.
- `pc_debug_gcc_clang_tidy` compiles the four queue/message tests without new warnings. `test_multi_ringbuff_mpmc.cpp` triggers pre-existing `modernize-use-nodiscard` warnings in `containers/paraos_ringbuff.hpp` (unchanged by this phase).

## Decisions

- Kept `paraos::sleep_for` as a single root-level header with `#ifdef PARAOS_LIKE_FREERTOS` rather than port-specific files, because the implementation is trivial and the project root is already in the include path.
- Kept `paraos::CriticalSection` in the migrated tests (deferred to Phase 21, already complete).

## Artifacts

- New file:
  - `paraos_sleep.hpp`
- Modified files:
  - `containers/tests/test_queue_blocking_spmc.cpp`
  - `containers/tests/test_queue_blocking_mpsc.cpp`
  - `containers/tests/test_queue_blocking_mpmc.cpp`
  - `containers/tests/test_multi_ringbuff_mpmc.cpp`
  - `containers/tests/test_message_multithread_many_producer_many_consumers.cpp`

## Next Phase

Phase 23: Build, tests and static analysis — full preset matrix verification and clang-tidy cleanup.
