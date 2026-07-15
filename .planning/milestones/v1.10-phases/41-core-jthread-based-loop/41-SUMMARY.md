---
phase: 41
phase_name: Core jthread-based loop
status: complete
completed_at: 2026-06-25
---

# Phase 41 Summary: Core jthread-based loop

## What was done

- Implemented the timer worker loop inside `port_unix/paraos_timer.hpp` using `paraos::jthread`, `paraos::mutex`, `paraos::binary_semaphore`, and `std::chrono::steady_clock`.
- Added a private `timer_loop(paraos::stop_token)` method that:
  - Waits for the next deadline using `wake_sem_.try_acquire_for(remaining_time)` without holding `mutex_`.
  - Calls the user-provided virtual `run()` with `mutex_` released.
  - Handles periodic mode by advancing `next_deadline_` by `period_ms_` with a one-period catch-up cap to avoid drift bursts.
  - Handles one-shot mode by invoking `run()` once and exiting.
  - Catches exceptions from `run()` and stops the timer without rethrowing.
- Implemented lazy worker creation in `start()`: `std::optional<paraos::jthread> worker_` is emplaced on first `start()` and reused on subsequent `start()` calls.
- Implemented `stop()` to set stop flags, wake the worker, and join it by assigning `worker_ = std::nullopt`.
- Implemented `change_period()` to update `period_ms_` and, if running, recompute `next_deadline_` from `now()` and wake the worker.
- Added `wake_worker()` helper to safely signal the binary semaphore without overflowing its counter.

## Verification results

- `port_unix/paraos_timer.hpp` contains no `timer_create`, `timer_settime`, `timer_delete`, `pthread_mutex_*`, `pthread_cond_*`, `pthread_create`, or `pthread_join` references.
- `port_unix/paraos_timer.hpp` contains no `#ifdef __linux__`, `#elif defined(__APPLE__)`, or platform-specific `#else` branches.
- Public API of `paraos::timer` is unchanged.
- `pc_debug_clang` and `pc_debug_gcc` build `test_timer` successfully.
- `ctest` reports 60/60 tests passing in both `pc_debug_clang` and `pc_debug_gcc`.
- `pc_debug_gcc_clang_tidy` builds `test_timer` without new warnings.
- FreeRTOS presets (`freertos_debug_clang`, `freertos_debug_gcc`) still compile (`ninja: no work to do`).
- Only `port_unix/paraos_timer.hpp` and `.planning/STATE.md` are modified.

## Artifacts

### Files modified
- `port_unix/paraos_timer.hpp`

### Symbols / behavior introduced
- Private member `std::chrono::steady_clock::time_point next_deadline_`.
- Private method `void timer_loop(const paraos::stop_token& token)`.
- Private helper `void wake_worker()`.
- Implemented `start()`, `stop()`, `change_period()` with jthread-based worker lifecycle.
- Worker loop with drift-corrected periodic scheduling and one-shot mode.

## Decisions / notes

- `wake_worker()` drains the binary semaphore before releasing to avoid `semaphore release overflow` from repeated wakeups.
- Catch-up is capped to one period to prevent bursts after long stalls.
- `run()` is always invoked without holding `mutex_`.
- Exceptions from `run()` stop the timer silently (no rethrow), preserving the non-throwing public API.

## Next phase

Phase 42: Start/stop/reset/change_period synchronization — harden edge cases such as `stop()` called from inside `run()`, destructor safety, and repeated wakeups.
