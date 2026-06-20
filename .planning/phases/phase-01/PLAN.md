# Phase 1 Plan: Diagnose and Fix macOS Compilation

**Phase:** 1 of 4 — Diagnose and Fix macOS Compilation
**Goal:** `port_unix/` compiles cleanly on macOS Clang without breaking Linux builds.
**Depends on:** None
**Requirements:** MAC-01, MAC-02, MAC-03
**Mode:** Standard
**Research:** `.planning/phases/phase-01/RESEARCH.md`

## Success Criteria

1. `cmake --preset pc_debug_clang` configures on macOS when using a macOS-targeting Clang.
2. `cmake --build build/pc_debug_clang/` completes with no errors in `port_unix/`.
3. Linux code paths compile unchanged when `__linux__` is defined.
4. All changes are isolated inside `port_unix/` using `__APPLE__` / `__linux__` preprocessor guards.

## Research Summary

- macOS does not implement POSIX interval timers (`timer_create`, `timer_settime`, `timer_delete`, `timer_t`, `itimerspec`, `SIGEV_THREAD`).
- macOS does not implement timed synchronization primitives (`pthread_mutex_timedlock`, `sem_timedwait`).
- The default `clang` in `PATH` on this machine targets Linux (`aarch64-unknown-linux-gnu`), causing CMake configure to fail. Validation must use `/usr/bin/clang` or another macOS-targeting toolchain.
- All other `port_unix/` primitives (`pthread_*`, `sem_*` without timeout, BSD sockets, `clock_gettime`) are available on macOS.

See full findings in `.planning/phases/phase-01/RESEARCH.md`.

---

## Plan 01-01: Audit `port_unix/` and Lock the Compatibility Strategy

**Owner:** Implementer
**Estimated effort:** Low
**Input:** `port_unix/*.hpp`, `RESEARCH.md`
**Output:** A confirmed strategy document in this plan; no code changes.

### Tasks

1. **Confirm the API surface.**
   - Read every header in `port_unix/` and list every POSIX/Linux-only symbol.
   - Cross-check the list against the probe results in `RESEARCH.md`.
   - Ensure no symbols outside `port_unix/` need to change for macOS compilation.

2. **Select the implementation approach for each missing API.**
   - **Timer:** Replace `SIGEV_THREAD`-based POSIX timers with a dedicated `pthread` per `Timer` that waits on a condition variable and calls `Run()`. Keep the same public constructor/destructor/Start/Stop/ChangePeriod/Reset semantics.
   - **Mutex timeout:** Replace `pthread_mutex_timedlock` with a macOS-only helper that loops on `pthread_mutex_trylock` and sleeps until the deadline.
   - **Semaphore timeout:** Replace `sem_timedwait` with a macOS-only helper that loops on `sem_trywait` and sleeps until the deadline.
   - **Toolchain:** Use explicit system Clang paths for validation; do not modify presets in this phase.

3. **Define the platform-isolation rules.**
   - Linux path: keep exactly the current code under `#ifdef __linux__`.
   - macOS path: add the replacement code under `#ifdef __APPLE__`.
   - If neither is defined, produce a clear `#error` so unsupported Unix variants fail fast.
   - Do not introduce runtime branches where compile-time selection is possible.

4. **Document behavior-parity assumptions.**
   - Timer callbacks continue to run in a dedicated thread, not a signal handler.
   - Millisecond-level accuracy is acceptable for the PC/test port.
   - `max_delay` still maps to an infinite wait; `0` still maps to a non-blocking attempt.

### Verification for 01-01

- [ ] Audit checklist of POSIX symbols is complete and reviewed.
- [ ] Implementation approach is recorded in `PLAN.md` (this document).
- [ ] No public API signatures are changed.

---

## Plan 01-02: Implement macOS-Compatible Replacements

**Owner:** Implementer
**Estimated effort:** Medium
**Input:** Strategy from 01-01
**Output:** Modified `port_unix/paraos_timer.hpp`, `port_unix/paraos_mutex.hpp`, `port_unix/paraos_semaphore.hpp`

### Task 1 — Timer: macOS backend in `port_unix/paraos_timer.hpp`

**Scope:** Add an `#ifdef __APPLE__` backend for the `Timer` class.

**Details:**

1. Keep the existing Linux backend intact under `#ifdef __linux__`.
2. Under `#ifdef __APPLE__`:
   - Replace `timer_t timer_id_` with a small control structure containing:
     - `pthread_t thread_{}`
     - `pthread_mutex_t mutex_{}`
     - `pthread_cond_t cond_{}`
     - `bool is_running_{false}`
     - `bool is_stop_requested_{false}`
     - `std::size_t period_ms_{0}`
     - `bool is_auto_reload_{false}`
   - In `Create()`:
     - Initialize `mutex_` and `cond_`.
     - Do **not** start the thread yet (matches current Linux behavior where `timer_create` only registers the handler).
   - In `Start()`:
     - If already running, update `period_ms_` and signal `cond_` to wake the thread with the new deadline.
     - If not running, set `is_running_ = true`, create the thread with `pthread_create`, and pass `this` as the argument.
   - In the thread routine:
     - Loop while `is_running_`.
     - Compute the next absolute deadline from `clock_gettime(CLOCK_MONOTONIC)` or `CLOCK_REALTIME` plus `period_ms_`.
     - Wait on `cond_` with `pthread_cond_timedwait`.
     - If woken by `Stop()`/`Reset()`/`ChangePeriod`, recompute and continue.
     - On timeout, call `Run()`.
     - If `is_auto_reload_` is false, set `is_running_ = false` and exit the loop.
   - In `Stop()`:
     - Set `is_stop_requested_ = true`, signal `cond_`, and join the thread if it was started.
   - In `ChangePeriod()`:
     - Update `period_ms_` and call `Start()` semantics.
   - In `Reset()`:
     - Call `Start()` semantics.
   - In the destructor:
     - Request stop, signal `cond_`, join the thread, destroy `cond_` and `mutex_`.

**Constraints:**

- Do not change the public constructor signature or default parameters.
- Preserve the `is_auto_reload_ == false` behavior of firing once after `period_ms_`.
- Do not leak the thread on destruction or rapid Start/Stop cycles.

### Task 2 — Mutex: macOS timed-lock helper in `port_unix/paraos_mutex.hpp`

**Scope:** Replace `pthread_mutex_timedlock` for finite timeouts on macOS.

**Details:**

1. Keep the Linux branch under `#ifdef __linux__`.
2. Under `#ifdef __APPLE__`:
   - Add a private helper `TryLockUntil(timespec deadline)` or `LockWithTimeout(std::size_t timeout_ms)`.
   - Implementation:
     - If `timeout_ms == 0`, call `pthread_mutex_trylock` and return immediately.
     - If `timeout_ms == max_delay`, call `pthread_mutex_lock` and block forever.
     - Otherwise:
       - Capture start time with `clock_gettime(CLOCK_MONOTONIC)` (or `CLOCK_REALTIME`).
       - Loop calling `pthread_mutex_trylock`.
       - On success, return 0.
       - On failure, compute elapsed time; if elapsed >= timeout, return `ETIMEDOUT`.
       - Sleep a short fixed interval (e.g., 1 ms) with `usleep` or `nanosleep`.
   - Use the helper in `Lock()` exactly where `pthread_mutex_timedlock` was called.

**Constraints:**

- Do not change `pthread_mutex_t` storage or initialization.
- Preserve the `lock_cnt_` logic and the `is_mutex_ready_` checks.

### Task 3 — Semaphore: macOS timed-wait helper in `port_unix/paraos_semaphore.hpp`

**Scope:** Replace `sem_timedwait` for finite timeouts on macOS.

**Details:**

1. Keep the Linux branch under `#ifdef __linux__`.
2. Under `#ifdef __APPLE__`:
   - Add a private helper `TryTakeUntil(timespec deadline)` or `TakeWithTimeout(std::size_t timeout_ms)`.
   - Implementation mirrors the mutex helper:
     - `timeout_ms == 0` → `sem_trywait`.
     - `timeout_ms == max_delay` → `sem_wait`.
     - Finite timeout → loop with `sem_trywait`, deadline check, and short sleep.
   - Use the helper in `SemaphoreBase::Take()`.

**Constraints:**

- Preserve `sem_t` storage and initialization.
- Preserve the `from_isr` assertion and `is_sem_created_` checks.

### Task 4 — Build validation on macOS

**Scope:** Prove the changes compile with a macOS-targeting toolchain.

**Details:**

1. Remove or rename the previous failed configure directory to avoid cache issues:
   - `rm -rf build/pc_debug_clang`
2. Configure using the system Clang:
   - `cmake --preset pc_debug_clang -D CMAKE_C_COMPILER=/usr/bin/clang -D CMAKE_CXX_COMPILER=/usr/bin/clang++`
3. Build:
   - `cmake --build build/pc_debug_clang/`
4. If the build still fails due to compiler target, capture the error and decide whether to:
   - Adjust `CMAKE_C_FLAGS`/`CMAKE_CXX_FLAGS` with `-target arm64-apple-darwin`, or
   - Add a note that the user must put `/usr/bin` before the Arm toolchain in `PATH`.

### Task 5 — Linux path regression check

**Scope:** Ensure the Linux code path was not accidentally altered.

**Details:**

1. Inspect the diff of `port_unix/` to confirm `#ifdef __linux__` blocks match the original logic.
2. If possible, run a Linux build in Docker or CI to confirm no regressions.
3. At minimum, visually verify that no Linux-only symbols were removed or renamed.

---

## Risks and Dependencies

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| `pthread_cond_timedwait` on macOS uses wall-clock time and can drift | Medium | Low | Use `CLOCK_MONOTONIC` for deadline computation; document that PC port is for testing, not hard-real-time |
| Thread-per-timer macOS backend races with rapid Start/Stop | Medium | Medium | Use mutex + condition variable for all state transitions; join thread in destructor |
| Compiler target mismatch cannot be resolved without preset changes | Medium | High | Validate with explicit system Clang; escalate to Phase 4 if presets must be modified |
| Retry-loop timeouts introduce CPU usage or latency | Low | Low | Use 1 ms sleeps; acceptable for test port |
| `clang-tidy` flags the new macOS-only code | Medium | Medium | Follow project `.clang-tidy`; suppress only locally with `// NOLINT` if justified |

## Verification Plan

### Automated / Build

- [ ] `cmake --preset pc_debug_clang` configures successfully with system Clang.
- [ ] `cmake --build build/pc_debug_clang/` compiles all `port_unix/` sources without errors.
- [ ] `cmake --build build/pc_debug_clang/` compiles all tests without errors.

### Static / Diff

- [ ] Linux code paths under `#ifdef __linux__` are unchanged in behavior.
- [ ] macOS code paths are isolated under `#ifdef __APPLE__`.
- [ ] No files outside `port_unix/` were modified.
- [ ] Public header signatures in `port_unix/` are unchanged.

### Manual

- [ ] Review thread lifecycle in the new timer backend for leaks and use-after-free.
- [ ] Review timeout helpers for overflow when `timeout_ms` is near `max_delay`.

## Definition of Done

- [ ] All Phase 1 success criteria are met.
- [ ] `RESEARCH.md` and `PLAN.md` are committed to `.planning/phases/phase-01/`.
- [ ] Code changes are committed and the macOS build passes.
- [ ] Linux code paths remain unmodified in behavior.
- [ ] `ROADMAP.md` Phase 1 plan checkboxes for `01-01` and `01-02` are marked complete.
- [ ] `STATE.md` is updated to reflect Phase 1 planned/ready status.

## Notes for Phase 2

- Phase 2 will run the existing `port_tests/` on macOS and compare results with the expected Linux behavior.
- Pay special attention to timer accuracy and mutex/semaphore timeout semantics when verifying behavior parity.
