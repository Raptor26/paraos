# Phase 32: Migrate `ThreadSequence` to `paraos::jthread` - Verification

**Status:** passed
**Date:** 2026-06-23
**Preset:** pc_debug_clang

## Automated Verification

- [x] `extra/paraos_thread_sequence.hpp` compiles with `pc_debug_clang`.
- [x] `IThreadSequence` owns a `paraos::jthread` member instead of `paraos::Thread`.
- [x] The timer tick loop (`Run()`) runs inside the jthread callable and observes `paraos::stop_token`.
- [x] `Finish()` requests stop, unblocks the sequence semaphore via `NotifyGive(false)`, and joins gracefully.
- [x] Deferred-delete (`is_dynamic` / `Base*`) logic from legacy `Thread::Finished()` is removed.
- [x] `NotifyGive(bool is_isr)` continues to wake the sequence thread.
- [x] Public API (`Register`, `Unregister`, `SetFreq`, `GetMainFreq`, `GiveRegisteredDelegatesNumb`) preserved.
- [x] `extra/tests/test_paraos_thread_sequence.cpp` standalone test passes.
- [x] No regressions in `[PARAOS EXTRA]` GoogleTest suite or Phase 31 standalone test.

## Test Results

```
Test project /Users/raptor/_vcs/paraos/build/pc_debug_clang
    Start 50: [PARAOS EXTRA]:Cooperative.Create
    ...
    Start 56: test_paraos_thread_sequence
1/1 Test #56: test_paraos_thread_sequence ......   Passed    0.71 sec

100% tests passed, 0 tests failed out of 1
```

All 8 relevant tests passed.

## Notes

- `GyrAcc::Update()` no longer calls `Finish()` from inside the sequence thread; it only sets the completion atomic, and a dedicated stopper jthread calls `Finish()` to avoid `thread::join` deadlock.
- Added `#include <thread>` to `paraos_sleep.hpp` so that `paraos::sleep_for` compiles when consumers no longer transitively include `<thread>` via legacy `paraos_thread.hpp`.

## Release Criteria

- No new compiler warnings or clang-tidy diagnostics in modified files.
- All PC presets remain green (validated on `pc_debug_clang`; remaining presets covered in Phase 35).
