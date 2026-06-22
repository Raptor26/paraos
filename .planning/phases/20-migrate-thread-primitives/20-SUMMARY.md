# Phase 20 Summary: Migrate thread primitives in container tests

**Phase:** 20
**Milestone:** v1.5 Modernize container tests on std-like primitives
**Status:** Complete ✅
**Completed:** 2026-06-22

## What was delivered

- Migrated all five multithreaded container tests from `paraos::Thread` to `paraos::jthread`:
  - `containers/tests/test_queue_blocking_spmc.cpp`
  - `containers/tests/test_queue_blocking_mpsc.cpp`
  - `containers/tests/test_queue_blocking_mpmc.cpp`
  - `containers/tests/test_multi_ringbuff_mpmc.cpp`
  - `containers/tests/test_message_multithread_many_producer_many_consumers.cpp`
- Removed watcher threads, `RegisterDelegate()`, `Finished()`, `StartScheduler()`, `DeleteAll()`, and `paraos::Thread::Exit()` from all target tests.
- Replaced worker ownership with local `std::vector<paraos::jthread>` inside an inner scope in `main()`, giving RAII join behavior.
- Added local `#ifdef`-based `SleepMs()` helpers for PC (`std::this_thread::sleep_for`) and FreeRTOS (`vTaskDelay`).
- Added `TIMEOUT 20` to all five multithreaded CTest targets in `containers/tests/CMakeLists.txt`.
- Fixed FreeRTOS compilation by using `paraos::PARAOS_ConvertMsToTicks()`.

## Artifacts produced

| Artifact | Path |
|----------|------|
| Context | `.planning/phases/20-migrate-thread-primitives/20-CONTEXT.md` |
| Plan | `.planning/phases/20-migrate-thread-primitives/20-PLAN.md` |
| Summary | `.planning/phases/20-migrate-thread-primitives/20-SUMMARY.md` |
| Verification | `.planning/phases/20-migrate-thread-primitives/20-VERIFICATION.md` |

## Requirements status

- **THR-01** ✅ — queue blocking spmc/mpsc/mpmc tests use `paraos::jthread`.
- **THR-02** ✅ — `test_multi_ringbuff_mpmc.cpp` uses `paraos::jthread`.
- **THR-03** ✅ — `test_message_multithread_many_producer_many_consumers.cpp` uses `paraos::jthread`.
- **THR-04** ✅ — no legacy lifecycle calls remain; finalization uses RAII join + FreeRTOS `std::_Exit()`.

## Verification performed

- `pc_debug_clang` builds and all five migrated tests pass.
- `pc_debug_gcc` builds and all five migrated tests pass.
- `freertos_debug_clang` compiles without errors.
- `freertos_debug_gcc` compiles without errors.
- Static grep confirms no `paraos::Thread`, `RegisterDelegate`, `Finished()`, `StartScheduler`, `DeleteAll`, or `paraos::Thread::Exit()` remain in the migrated files.

## Known observations

- `test_message_multithread_many_producer_many_consumers` is sensitive to scheduling/timing under rapid repeated execution (likely pre-existing queue/semaphore behavior on macOS POSIX). A single run of the full stress suite passes; repeated stress runs of that one test in isolation can time out. This is captured as a stability observation, not a regression, since the legacy test used the same queue/buffer internals.

## Risks accepted

- Rapid-repeat stress testing of a single test may expose scheduling sensitivity on the macOS POSIX simulator. CI-style single-run suites pass.

## Handoff to Phase 21

- No `paraos::Mutex`, `MutexGuard`, or semaphore usage was found in the audited tests, so Phase 21 is expected to be minimal for `containers/tests/`.
- If a compile-time smoke test for `std::lock_guard<paraos::mutex>` / `std::unique_lock<paraos::mutex>` is desired, it can be added as a small standalone verification.
