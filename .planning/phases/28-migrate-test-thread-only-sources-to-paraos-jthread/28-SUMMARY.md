---
phase: 28
status: complete
requirements-completed:
  - MIG-01
  - MIG-02
  - MIG-03
  - MIG-04
  - LIFE-01
  - LIFE-02
  - LIFE-03
  - IDIO-01
---

# Phase 28 Summary: Migrate `test_thread_only_*` sources to `paraos::jthread`

## What Was Done

- Rewrote `port_tests/test_thread_only_global.cpp` to use global `paraos::jthread` objects and worker lambdas.
- Rewrote `port_tests/test_thread_only_static.cpp` to use `const static paraos::jthread` objects.
- Rewrote `port_tests/test_thread_only_stack.cpp` to store worker threads in `std::vector<paraos::jthread>`.
- Rewrote `port_tests/test_thread_only_stack_with_multiple_threads.cpp` to use a `MyThreadGroup` struct owning three `paraos::jthread` objects and two `paraos::binary_semaphore` objects, stored in `std::vector<std::unique_ptr<MyThreadGroup>>`.
- Applied the unified lifecycle pattern in all four files: `start_scheduler()` → worker threads → `stopper` → `NotifySchedulerEnded()` → `IdleHook()` → `end_scheduler()`.
- Added `#if PARAOS_LIKE_FREERTOS` assignment of `paraos::freertos_idle_fnc_ptr = IdleHook`.
- Replaced `paraos::Thread::DelayMs()` with `paraos::sleep_for(std::chrono::milliseconds{...})`.
- Preserved `PrintDebug` with `paraos::CriticalSection`.
- Removed `paraos::Thread::StartScheduler()`, `paraos::Thread::Exit()`, `paraos::Thread::DeleteAll()`, and `std::_Exit()`.

## Verification

- `pc_debug_clang`: all four executables build and print `OK`.
- `pc_debug_gcc_clang_tidy`: builds cleanly with clang-tidy enabled.
- `freertos_debug_clang` and `freertos_debug_gcc`: all four targets compile.
- No occurrences of legacy thread API remain in the four files.

## Files Changed

- `port_tests/test_thread_only_global.cpp`
- `port_tests/test_thread_only_static.cpp`
- `port_tests/test_thread_only_stack.cpp`
- `port_tests/test_thread_only_stack_with_multiple_threads.cpp`
