---
wave: 41
depends_on: [40]
files_modified:
  - port_unix/paraos_timer.hpp
autonomous: true
requirements:
  - TMR-01
  - TMR-02
  - TMR-03
  - TMR-05
  - TMR-06
  - TMR-11
  - TMR-12
---

# Phase 41: Core jthread-based loop

**Goal:** Реализовать рабочий поток таймера поверх PARAOS-примитивов, поддерживающий периодический и one-shot режимы без дрейфа.

**Milestone:** PARAOS v1.10 — Modernize Unix timer with paraos primitives.

**Requirements covered:** TMR-01, TMR-02, TMR-03, TMR-05, TMR-06, TMR-11, TMR-12

**Phase boundary:** This phase implements the timer worker loop inside `port_unix/paraos_timer.hpp` only. Synchronization edge cases for `stop()` from inside `run()` and full destructor safety are intentionally left for Phase 42; expanded runtime accuracy tests are left for Phase 43.

---

## Tasks

<tasks>

<task>
  <id>41.1</id>
  <title>Implement private timer worker loop in `port_unix/paraos_timer.hpp`</title>
  <requirements>TMR-01, TMR-02, TMR-03, TMR-05, TMR-06, TMR-11</requirements>
  <read_first>
    - port_unix/paraos_timer.hpp
    - port_pc/paraos_jthread.hpp
    - port_pc/paraos_semaphore_std.hpp
    - port_pc/paraos_mutex_std.hpp
    - paraos_sleep.hpp
    - extra/paraos_oneshot_executor.hpp
    - AGENTS.md
  </read_first>
  <action>
    Modify `port_unix/paraos_timer.hpp` to add and implement a private worker loop:
    1. Add `#include <chrono>` if not already present.
    2. Add private member `std::chrono::steady_clock::time_point next_deadline_{}`.
    3. Add private method `void timer_loop(paraos::stop_token token)` with the following behavior:
       a. Loop while `!token.stop_requested()` and `!is_stop_requested_` (read under `mutex_`).
       b. Inside the loop, under `mutex_`, capture `period_ms_` and `is_auto_reload_` into local variables, then release the lock.
       c. Compute remaining time until `next_deadline_` from `std::chrono::steady_clock::now()`. If the deadline has already passed, treat remaining as zero.
       d. Call `wake_sem_.try_acquire_for(remaining_time)` to wait. Do not hold `mutex_` during this call.
       e. After waking, re-acquire `mutex_` and check `is_stop_requested_`; if true, exit the loop.
       f. If `std::chrono::steady_clock::now()` has not yet reached `next_deadline_` (spurious early wake), continue the loop without calling `run()`.
       g. Release `mutex_` before calling `this->run()`.
       h. After `run()` returns, re-acquire `mutex_`.
       i. If `is_auto_reload_ == false`, set `is_running_ = false` and exit the loop.
       j. If `is_auto_reload_ == true`, advance `next_deadline_` by `period_ms_`. If the new deadline is already in the past, cap catch-up so the deadline is at most one period behind `now()` (i.e., do not schedule bursts of catch-up calls).
       k. Wrap the `run()` call in a `try/catch(...)` block: if `run()` throws, set `is_running_ = false` and `is_stop_requested_ = true` under `mutex_`, then exit the loop without rethrowing.
    4. Ensure no `timer_create`, `timer_settime`, `timer_delete`, `pthread_mutex_*`, `pthread_cond_*`, `pthread_create`, `pthread_join` references are introduced.
    5. Ensure no `#ifdef __linux__`, `#elif defined(__APPLE__)`, or platform-specific `#else` branches are introduced.
  </action>
  <acceptance_criteria>
    - `grep -E 'void timer_loop\(paraos::stop_token' port_unix/paraos_timer.hpp` returns a match.
    - `grep -E 'std::chrono::steady_clock::time_point next_deadline_' port_unix/paraos_timer.hpp` returns a match.
    - `grep -E 'run\(\)' port_unix/paraos_timer.hpp` appears only inside the worker loop or the public virtual method declaration.
    - `grep -E 'timer_create|timer_settime|timer_delete|pthread_mutex_|pthread_cond_|pthread_create|pthread_join' port_unix/paraos_timer.hpp` returns no matches.
    - `grep -E '#ifdef __linux__|#elif defined\(__APPLE__\)' port_unix/paraos_timer.hpp` returns no matches.
    - `wake_sem_.try_acquire_for(` appears at least once in `port_unix/paraos_timer.hpp`.
    - The body of `run()` is not called while `std::scoped_lock` or `std::unique_lock` on `mutex_` is in scope.
  </acceptance_criteria>
</task>

<task>
  <id>41.2</id>
  <title>Implement lazy `start()` creating `std::optional<paraos::jthread>` worker</title>
  <requirements>TMR-05, TMR-06, TMR-11, TMR-12</requirements>
  <read_first>
    - port_unix/paraos_timer.hpp
    - port_pc/paraos_jthread.hpp
    - extra/paraos_oneshot_executor.hpp
  </read_first>
  <action>
    Rewrite `start()` in `port_unix/paraos_timer.hpp` so that:
    1. It ignores `max_block_time` and `is_isr` via `PARAOS_ATTR_UNUSED_VAR` (backward-compatibility parameters).
    2. It acquires `mutex_` (e.g., `std::scoped_lock lock{mutex_}`).
    3. If `is_running_ == true`:
       a. Recompute `next_deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(period_ms_)`.
       b. Call `wake_sem_.release()` to wake the existing worker.
       c. Return `isr_bool{true}`.
    4. If `is_running_ == false`:
       a. Set `is_running_ = true`.
       b. Set `is_stop_requested_ = false`.
       c. Set `next_deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(period_ms_)`.
       d. Emplace the optional worker: `worker_.emplace([this](const paraos::stop_token& token) { timer_loop(token); });`.
       e. Return `isr_bool{true}`.
    5. Do not create the worker outside this branch; the `worker_` member remains empty until the first successful `start()`.
  </action>
  <acceptance_criteria>
    - `grep -E 'worker_\.emplace' port_unix/paraos_timer.hpp` returns a match.
    - `grep -E 'is_running_' port_unix/paraos_timer.hpp` appears in `start()` and in the worker loop.
    - `grep -E 'next_deadline_ = std::chrono::steady_clock::now\(\) \+ std::chrono::milliseconds\(period_ms_\)' port_unix/paraos_timer.hpp` returns at least one match.
    - `start()` returns `isr_bool{true}` explicitly.
    - `cmake --build build/pc_debug_clang --target test_timer` succeeds.
    - `cmake --build build/pc_debug_gcc --target test_timer` succeeds.
  </acceptance_criteria>
</task>

<task>
  <id>41.3</id>
  <title>Implement `stop()`, `change_period()`, and keep `reset()` forwarding</title>
  <requirements>TMR-01, TMR-02, TMR-03, TMR-05, TMR-11</requirements>
  <read_first>
    - port_unix/paraos_timer.hpp
    - port_pc/paraos_jthread.hpp
  </read_first>
  <action>
    Update the public control methods in `port_unix/paraos_timer.hpp`:
    1. `stop()`:
       a. Acquire `mutex_`.
       b. Set `is_stop_requested_ = true`.
       c. Set `is_running_ = false`.
       d. Call `wake_sem_.release()`.
       e. Release the lock (via scope exit).
       f. Assign `worker_ = std::nullopt;` to request stop and join the worker through `paraos::jthread` destructor semantics.
       g. Return `isr_bool{true}`.
    2. `change_period(std::size_t period_ms, ...)`:
       a. Acquire `mutex_`.
       b. Update `period_ms_ = period_ms`.
       c. If `is_running_ == true`, recompute `next_deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(period_ms_)` and call `wake_sem_.release()`.
       d. Release the lock.
       e. Return `isr_bool{true}`.
    3. `reset(...)`:
       a. Keep the existing body that forwards to `start(max_block_time, is_isr)`.
    4. In the destructor `~timer()`, keep the call to `stop()`.
  </action>
  <acceptance_criteria>
    - `grep -E 'worker_ = std::nullopt' port_unix/paraos_timer.hpp` returns a match inside `stop()`.
    - `grep -E 'is_stop_requested_ = true' port_unix/paraos_timer.hpp` returns a match inside `stop()`.
    - `grep -E 'period_ms_ = period_ms' port_unix/paraos_timer.hpp` returns a match inside `change_period()`.
    - `grep -E 'return start\(max_block_time, is_isr\)' port_unix/paraos_timer.hpp` returns a match inside `reset()`.
    - `stop()` does not call `pthread_join`, `pthread_cancel`, `timer_delete`, or any POSIX/pthread API directly.
    - `change_period()` calls `wake_sem_.release()` when `is_running_ == true`.
  </acceptance_criteria>
</task>

<task>
  <id>41.4</id>
  <title>Verify builds, static analysis, and basic runtime behavior</title>
  <requirements>TMR-01, TMR-02, TMR-03, TMR-05, TMR-06, TMR-11, TMR-12</requirements>
  <read_first>
    - port_unix/paraos_timer.hpp
    - port_tests/test_timer.cpp
    - port_tests/CMakeLists.txt
    - AGENTS.md
  </read_first>
  <action>
    Validate the implementation with the existing scaffold test:
    1. Configure and build `pc_debug_clang` and `pc_debug_gcc`.
    2. Build the `test_timer` target in both presets.
    3. Run `ctest --test-dir build/pc_debug_clang -R test_timer --output-on-failure`.
    4. Run `ctest --test-dir build/pc_debug_gcc -R test_timer --output-on-failure`.
    5. If a `*_clang_tidy` preset is available (e.g., `pc_debug_gcc_clang_tidy`), configure and build it; confirm `port_unix/paraos_timer.hpp` and `port_tests/test_timer.cpp` produce no new warnings.
    6. Confirm `port_win/paraos_timer.hpp` and `port_freertos/paraos_timer.hpp` are untouched (`git diff --name-only` does not list them).
    7. Run `grep` checks for forbidden POSIX/pthread symbols and platform macros (see acceptance criteria in tasks 41.1–41.3).
  </action>
  <acceptance_criteria>
    - `cmake --build build/pc_debug_clang --target test_timer` exits with code 0.
    - `cmake --build build/pc_debug_gcc --target test_timer` exits with code 0.
    - `ctest --test-dir build/pc_debug_clang -R test_timer --output-on-failure` reports `1 test passed`.
    - `ctest --test-dir build/pc_debug_gcc -R test_timer --output-on-failure` reports `1 test passed`.
    - `git diff --name-only` does not contain `port_win/paraos_timer.hpp` or `port_freertos/paraos_timer.hpp`.
    - `grep -E 'timer_create|timer_settime|timer_delete|pthread_mutex_|pthread_cond_|pthread_create|pthread_join' port_unix/paraos_timer.hpp` returns empty output.
    - `grep -E '#ifdef __linux__|#elif defined\(__APPLE__\)' port_unix/paraos_timer.hpp` returns empty output.
    - If `pc_debug_gcc_clang_tidy` (or another available `*_clang_tidy` preset) is built, the build completes without errors attributed to `port_unix/paraos_timer.hpp` or `port_tests/test_timer.cpp`.
  </acceptance_criteria>
</task>

</tasks>

---

## must_haves

1. `port_unix/paraos_timer.hpp` contains no `timer_create`, `timer_settime`, `timer_delete`, `pthread_mutex_*`, `pthread_cond_*`, `pthread_create`, or `pthread_join` references.
2. `port_unix/paraos_timer.hpp` contains no `#ifdef __linux__`, `#elif defined(__APPLE__)`, or platform-specific `#else` branches.
3. The public API of `paraos::timer` is unchanged: constructor `(std::size_t period_ms, bool start_immediately = false, bool is_auto_reload = true, std::string_view name = "Timer")`; methods `start()`, `stop()`, `reset()`, `change_period()`; virtual `run()`; deleted copy/move; deprecated aliases.
4. A `std::optional<paraos::jthread> worker_` is created lazily inside `start()` and destroyed by `worker_ = std::nullopt` inside `stop()`.
5. The worker loop uses `std::chrono::steady_clock` to compute deadlines and `paraos::binary_semaphore::try_acquire_for()` to wait for the remaining time.
6. `mutex_` is never held while calling the user-provided virtual `run()`; it is only used to guard reads/writes of `period_ms_`, `is_running_`, `is_stop_requested_`, and `next_deadline_`.
7. `is_auto_reload_ == true` causes `run()` to be invoked repeatedly with drift-corrected deadlines (catch-up capped to one period).
8. `is_auto_reload_ == false` causes `run()` to be invoked exactly once after `period_ms_`, then sets `is_running_ = false` and exits the loop.
9. `change_period()` updates `period_ms_` under `mutex_` and, if running, recomputes `next_deadline_` from `now()` and wakes the worker via `wake_sem_.release()`.
10. `start()` on an already-running timer resets `next_deadline_` to `now() + period_ms_` and wakes the worker.
11. `port_win/paraos_timer.hpp` and `port_freertos/paraos_timer.hpp` are not modified.
12. `pc_debug_clang` and `pc_debug_gcc` build `test_timer` successfully, and the test passes in both presets.

---

## Verification

1. Configure PC presets and build the timer test:
   ```bash
   cmake --preset pc_debug_clang
   cmake --build build/pc_debug_clang --target test_timer
   cmake --preset pc_debug_gcc
   cmake --build build/pc_debug_gcc --target test_timer
   ```
2. Run the new test:
   ```bash
   ctest --test-dir build/pc_debug_clang -R test_timer --output-on-failure
   ctest --test-dir build/pc_debug_gcc -R test_timer --output-on-failure
   ```
3. Confirm no POSIX/pthread timer API remains:
   ```bash
   grep -E 'timer_create|timer_settime|timer_delete|pthread_mutex_|pthread_cond_|pthread_create|pthread_join' port_unix/paraos_timer.hpp
   ```
   Expected: empty output.
4. Confirm no platform split remains:
   ```bash
   grep -E '#ifdef __linux__|#elif defined\(__APPLE__\)' port_unix/paraos_timer.hpp
   ```
   Expected: empty output.
5. Confirm only the Unix timer header changed:
   ```bash
   git diff --name-only
   ```
   Expected: only `port_unix/paraos_timer.hpp` (plus any planning artifacts from GSD).
6. If available, run the clang-tidy preset:
   ```bash
   cmake --preset pc_debug_gcc_clang_tidy
   cmake --build build/pc_debug_gcc_clang_tidy --target test_timer
   ```
   Expected: no errors originating from `port_unix/paraos_timer.hpp` or `port_tests/test_timer.cpp`.

---

## Artifacts this phase produces

### Files modified
- `port_unix/paraos_timer.hpp`

### Symbols / behavior introduced
- Private member `std::chrono::steady_clock::time_point next_deadline_{}` in `paraos::timer`.
- Private method `void timer_loop(paraos::stop_token token)` in `paraos::timer`.
- Implemented `paraos::timer::start()`:
  - Lazily creates `worker_` via `worker_.emplace(...)` on first call.
  - Resets `next_deadline_` to `now() + period_ms_` and releases `wake_sem_` if already running.
- Implemented `paraos::timer::stop()`:
  - Sets `is_stop_requested_ = true` and `is_running_ = false` under `mutex_`.
  - Releases `wake_sem_`.
  - Joins worker by assigning `worker_ = std::nullopt`.
- Implemented `paraos::timer::change_period()`:
  - Updates `period_ms_` under `mutex_`.
  - Recomputes `next_deadline_` and releases `wake_sem_` when running.
- Worker loop behavior:
  - Waits on `wake_sem_.try_acquire_for(remaining_time)` without holding `mutex_`.
  - Calls virtual `run()` with `mutex_` released.
  - Periodic mode advances `next_deadline_` by `period_ms_` with one-period catch-up cap.
  - One-shot mode runs once and exits.
  - Catches exceptions from `run()` and stops the timer without rethrowing.

### Build / CLI surface
- `cmake --build build/pc_debug_clang --target test_timer`
- `cmake --build build/pc_debug_gcc --target test_timer`
- `ctest --test-dir build/pc_debug_clang -R test_timer`
- `ctest --test-dir build/pc_debug_gcc -R test_timer`
- Optional: `cmake --build build/pc_debug_gcc_clang_tidy --target test_timer`
