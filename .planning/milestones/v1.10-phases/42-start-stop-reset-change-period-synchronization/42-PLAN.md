---
wave: 42
depends_on: [41]
files_modified:
  - port_unix/paraos_timer.hpp
  - port_tests/test_timer.cpp
autonomous: true
requirements:
  - TMR-07
  - TMR-08
  - TMR-09
  - TMR-10
---

# Phase 42: Start/stop/reset/change_period synchronization

**Goal:** Обеспечить корректную синхронизацию публичных методов с рабочим потоком, безопасный stop изнутри `run()` и безопасный деструктор.

**Milestone:** PARAOS v1.10 — Modernize Unix timer with paraos primitives.

**Requirements covered:** TMR-07, TMR-08, TMR-09, TMR-10

**Phase boundary:** This phase hardens synchronization edge cases inside `port_unix/paraos_timer.hpp` and adds minimal safety checks to `port_tests/test_timer.cpp`. Full runtime accuracy/mode coverage tests are deferred to Phase 43.

---

## Tasks

<tasks>

<task>
  <id>42.1</id>
  <title>Audit current timer implementation and synchronization edge cases</title>
  <requirements>TMR-07, TMR-08, TMR-09, TMR-10</requirements>
  <read_first>
    - port_unix/paraos_timer.hpp
    - port_tests/test_timer.cpp
    - port_tests/CMakeLists.txt
    - .planning/phases/41-core-jthread-based-loop/41-SUMMARY.md
    - port_pc/paraos_jthread.hpp
    - port_pc/paraos_scheduler.hpp
    - AGENTS.md
  </read_first>
  <action>
    Read the listed files and confirm:
    1. The public API of `paraos::timer` is exactly the existing constructor plus `start()`, `stop()`, `reset()`, `change_period()`, and virtual `run()`.
    2. `port_win/paraos_timer.hpp` and `port_freertos/paraos_timer.hpp` are untouched.
    3. `paraos::jthread` destructor skips join only for the owner thread but still destroys the underlying context, so `timer::stop()` must avoid assigning `worker_ = std::nullopt` when invoked from inside `run()`.
    4. The post-run deadline advance in `timer_loop()` can double-add the period if `change_period()` or a repeated `start()` updates `next_deadline_` while `run()` is executing.
    Record the current state and edge-case list in execution notes.
  </action>
  <acceptance_criteria>
    - `git status --short` shows no modifications to `port_win/paraos_timer.hpp` or `port_freertos/paraos_timer.hpp`.
    - A written note exists stating that `stop()` from `run()` is unsafe if it executes `worker_ = std::nullopt`.
    - A written note exists stating that `change_period()` and a repeated `start()` can cause `timer_loop()` to add the period to an already-recomputed `next_deadline_`.
    - The public API signatures in `port_unix/paraos_timer.hpp` match the Phase 41 summary.
  </acceptance_criteria>
</task>

<task>
  <id>42.2</id>
  <title>Harden start() for repeated invocation</title>
  <requirements>TMR-07</requirements>
  <read_first>
    - port_unix/paraos_timer.hpp
  </read_first>
  <action>
    Modify `start()` in `port_unix/paraos_timer.hpp` so that, while holding `mutex_`, it:
    1. Sets `is_stop_requested_ = false;`.
    2. Recomputes `next_deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(period_ms_);`.
    3. If `is_running_` is already `true`, sets `period_changed_ = true;`, calls `wake_worker()`, and returns `isr_bool{true}` without touching `worker_`.
    4. Otherwise sets `is_running_ = true;`, sets `period_changed_ = false;`, and emplaces the worker with `[this](const paraos::stop_token& token) -> void { timer_loop(token); }`.
  </action>
  <acceptance_criteria>
    - `grep -n "is_stop_requested_ = false;" port_unix/paraos_timer.hpp` appears inside `start()`.
    - `grep -n "next_deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(period_ms_);" port_unix/paraos_timer.hpp` appears inside `start()`.
    - `grep -n "period_changed_ = true;" port_unix/paraos_timer.hpp` appears inside `start()`.
    - `worker_.emplace` does not appear inside the `is_running_` branch.
  </acceptance_criteria>
</task>

<task>
  <id>42.3</id>
  <title>Harden stop() against self-deadlock when called from run()</title>
  <requirements>TMR-09</requirements>
  <read_first>
    - port_unix/paraos_timer.hpp
    - port_pc/paraos_jthread.hpp
  </read_first>
  <action>
    1. Add `#include <atomic>` to `port_unix/paraos_timer.hpp` if not present.
    2. Add a private member `std::atomic<bool> is_in_run_{false};`.
    3. In `timer_loop()`, set `is_in_run_.store(true, std::memory_order_release);` immediately before the `try` block that calls `run()` and set it back to `false` with `std::memory_order_release` in both the normal and `catch(...)` paths.
    4. In `stop()`:
       - Under `const std::scoped_lock lock{mutex_}` set `is_stop_requested_ = true;`, `is_running_ = false;`, and call `wake_worker()`.
       - Release the lock.
       - Then execute `if (!is_in_run_.load(std::memory_order_acquire)) { worker_ = std::nullopt; }`.
       - Return `isr_bool{true}`.
    5. Do not add `std::this_thread::sleep_for`, `yield`, or any other delay.
  </action>
  <acceptance_criteria>
    - `grep -n "#include <atomic>" port_unix/paraos_timer.hpp` succeeds.
    - `grep -n "std::atomic<bool> is_in_run_" port_unix/paraos_timer.hpp` succeeds.
    - `grep -n "is_in_run_.store(true" port_unix/paraos_timer.hpp` and `grep -n "is_in_run_.store(false" port_unix/paraos_timer.hpp` both appear in `timer_loop()`.
    - `grep -n "if (!is_in_run_.load(std::memory_order_acquire))" port_unix/paraos_timer.hpp` appears before `worker_ = std::nullopt;` in `stop()`.
    - `grep -n "std::this_thread::sleep_for\|std::this_thread::yield" port_unix/paraos_timer.hpp` returns nothing.
  </acceptance_criteria>
</task>

<task>
  <id>42.4</id>
  <title>Harden change_period() for immediate application</title>
  <requirements>TMR-08</requirements>
  <read_first>
    - port_unix/paraos_timer.hpp
  </read_first>
  <action>
    Modify `change_period()` in `port_unix/paraos_timer.hpp` so that, while holding `mutex_`, it:
    1. Assigns `period_ms_ = period_ms;`.
    2. If `is_running_` is `true`, sets `next_deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(period_ms_);`, sets `period_changed_ = true;`, and calls `wake_worker()`.
    3. Returns `isr_bool{true}`.
  </action>
  <acceptance_criteria>
    - `grep -n "period_ms_ = period_ms;" port_unix/paraos_timer.hpp` appears inside `change_period()`.
    - `grep -n "next_deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(period_ms_);" port_unix/paraos_timer.hpp` appears inside `change_period()`.
    - `grep -n "period_changed_ = true;" port_unix/paraos_timer.hpp` appears inside `change_period()`.
    - `grep -n "wake_worker();" port_unix/paraos_timer.hpp` appears inside the `is_running_` block of `change_period()`.
  </acceptance_criteria>
</task>

<task>
  <id>42.5</id>
  <title>Verify reset() semantics and worker-loop deadline handling</title>
  <requirements>TMR-07, TMR-09</requirements>
  <read_first>
    - port_unix/paraos_timer.hpp
  </read_first>
  <action>
    1. Ensure `reset()` body remains exactly `return start(max_block_time, is_isr);`.
    2. Add a private member `bool period_changed_{false};` guarded by `mutex_`.
    3. Ensure `timer_loop()` after `wake_sem_.try_acquire_for(remaining)`:
       - Acquires `mutex_`.
       - Checks `if (is_stop_requested_) { break; }`.
       - Checks `if (std::chrono::steady_clock::now() < next_deadline_) { continue; }`.
    4. Ensure `timer_loop()` after `run()`:
       - Acquires `mutex_`.
       - Checks `is_stop_requested_` and `is_auto_reload_`.
       - If `period_changed_` is `true`, sets `next_deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(period_ms_);` and clears `period_changed_ = false;`.
       - Otherwise advances `next_deadline_ += std::chrono::milliseconds(period_ms_);` with the existing one-period catch-up cap.
    5. Ensure `wake_worker()` contains the drain loop `while (wake_sem_.try_acquire()) {}` followed by a single `wake_sem_.release();`.
    6. Ensure the destructor body remains exactly `stop();`.
  </action>
  <acceptance_criteria>
    - `grep -n "return start(max_block_time, is_isr);" port_unix/paraos_timer.hpp` appears inside `reset()`.
    - `grep -n "bool period_changed_{false};" port_unix/paraos_timer.hpp` succeeds.
    - `grep -n "if (is_stop_requested_) { break; }" port_unix/paraos_timer.hpp` appears inside `timer_loop()` after `try_acquire_for`.
    - `grep -n "if (std::chrono::steady_clock::now() < next_deadline_) { continue; }" port_unix/paraos_timer.hpp` appears inside `timer_loop()`.
    - `grep -n "if (period_changed_)" port_unix/paraos_timer.hpp` appears inside `timer_loop()` after `run()`.
    - `grep -n "while (wake_sem_.try_acquire()) {}" port_unix/paraos_timer.hpp` succeeds.
    - `grep -n "~timer() { stop(); }" port_unix/paraos_timer.hpp` succeeds.
  </acceptance_criteria>
</task>

<task>
  <id>42.6</id>
  <title>Add minimal safety checks to port_tests/test_timer.cpp</title>
  <requirements>TMR-09, TMR-10</requirements>
  <read_first>
    - port_tests/test_timer.cpp
    - port_unix/paraos_timer.hpp
    - port_pc/paraos_jthread.hpp
  </read_first>
  <action>
    Extend `port_tests/test_timer.cpp` with three minimal standalone checks and keep `main()` returning 0:
    1. A `self_stopping_timer` derived class whose `run()` calls `this->stop();` and sets an `std::atomic<bool> stopped_in_run_{false}` to `true`. In `main()`, call `paraos::jthread::start_scheduler();`, start the timer with a 200 ms period, wait up to 1000 ms for `stopped_in_run_` to become true using `paraos::sleep_for(std::chrono::milliseconds(10))` in a loop, then call `paraos::jthread::end_scheduler();`.
    2. A scope block where a `test_timer_app` instance is started and allowed to go out of scope, verifying the destructor does not hang.
    3. A loop of at least 3 iterations calling `start()`, `change_period(50)`, and `stop()` on the same timer object.
    Add necessary includes (`<atomic>`, `<chrono>`) and use `paraos::sleep_for`.
  </action>
  <acceptance_criteria>
    - `grep -n "this->stop();" port_tests/test_timer.cpp` appears inside an overridden `run()`.
    - `grep -n "paraos::jthread::start_scheduler();" port_tests/test_timer.cpp` succeeds.
    - `grep -n "paraos::jthread::end_scheduler();" port_tests/test_timer.cpp` succeeds.
    - `grep -n "change_period(50" port_tests/test_timer.cpp` succeeds.
    - A loop with `for (int i = 0; i < 3; ++i)` (or equivalent) contains `start()`, `change_period(...)`, and `stop()`.
    - `ctest --test-dir build/pc_debug_clang -R test_timer --output-on-failure` exits 0 with no timeout.
  </acceptance_criteria>
</task>

<task>
  <id>42.7</id>
  <title>Verify builds, tests, and clang-tidy cleanliness</title>
  <requirements>TMR-07, TMR-08, TMR-09, TMR-10</requirements>
  <read_first>
    - AGENTS.md
    - CMakePresets.json
  </read_first>
  <action>
    1. Configure and build `pc_debug_clang` and `pc_debug_gcc`.
    2. Run `ctest --test-dir build/pc_debug_clang --output-on-failure --stop-on-failure` and the same for `build/pc_debug_gcc`.
    3. Configure and build the available `*_clang_tidy` preset (e.g., `pc_debug_gcc_clang_tidy`) and confirm no new warnings/errors from `port_unix/paraos_timer.hpp` or `port_tests/test_timer.cpp`.
    4. Run `git diff --name-only` and ensure only `port_unix/paraos_timer.hpp` and `port_tests/test_timer.cpp` are modified.
  </action>
  <acceptance_criteria>
    - `ctest` reports 60/60 tests passed for `pc_debug_clang` and `pc_debug_gcc` (or the existing baseline count with no new failures).
    - The clang-tidy preset build completes without warnings attributed to `paraos_timer.hpp` or `test_timer.cpp`.
    - `git diff --name-only` output contains exactly:
      ```
      port_tests/test_timer.cpp
      port_unix/paraos_timer.hpp
      ```
  </acceptance_criteria>
</task>

</tasks>

---

## must_haves

1. `start()` on an already running timer resets `next_deadline_` to `now() + period_ms_`, wakes the worker, sets `period_changed_ = true`, and does not create a second thread.
2. `change_period()` atomically updates `period_ms_` and, when running, recomputes `next_deadline_` from `now()`, sets `period_changed_ = true`, and wakes the worker before the next cycle.
3. `stop()` sets `is_stop_requested_` and `is_running_` under `mutex_`, wakes the worker, releases the lock, and only assigns `worker_ = std::nullopt` when `stop()` is not being called from within `run()`.
4. `reset()` remains implemented as `return start(max_block_time, is_isr);`.
5. The destructor calls `stop()`.
6. `run()` is always invoked without holding `mutex_`; `std::atomic<bool> is_in_run_` tracks whether `run()` is active so `stop()` can avoid destroying the worker from within the worker thread.
7. `bool period_changed_` prevents double-adding the period after `change_period()` or a repeated `start()` while running.
8. `wake_worker()` drains the binary semaphore before releasing to prevent overflow on repeated wakeups.
9. The public API of `paraos::timer` is unchanged.
10. `port_win/paraos_timer.hpp` and `port_freertos/paraos_timer.hpp` are not modified.
11. `pc_debug_clang`, `pc_debug_gcc`, and the available `*_clang_tidy` preset build cleanly.
12. `port_tests/test_timer.cpp` contains minimal safety checks for self-stop, destructor safety, and repeated start/change_period/stop cycles.

---

## Verification

1. Configure, build, and test PC presets:
   ```bash
   cmake --preset pc_debug_clang
   cmake --build build/pc_debug_clang/
   ctest --test-dir build/pc_debug_clang/ --output-on-failure --stop-on-failure

   cmake --preset pc_debug_gcc
   cmake --build build/pc_debug_gcc/
   ctest --test-dir build/pc_debug_gcc/ --output-on-failure --stop-on-failure
   ```
   Expected: 60/60 tests passed in both presets.

2. Configure and build clang-tidy preset:
   ```bash
   cmake --preset pc_debug_gcc_clang_tidy
   cmake --build build/pc_debug_gcc_clang_tidy/
   ```
   Expected: No warnings or errors attributed to `port_unix/paraos_timer.hpp` or `port_tests/test_timer.cpp`.

3. Confirm only expected files changed:
   ```bash
   git diff --name-only
   ```
   Expected output:
   ```
   port_tests/test_timer.cpp
   port_unix/paraos_timer.hpp
   ```

4. Confirm forbidden symbols and platform macros are absent:
   ```bash
   grep -E 'timer_create|timer_settime|timer_delete|pthread_mutex_|pthread_cond_|pthread_create|pthread_join' port_unix/paraos_timer.hpp
   grep -E '#ifdef __linux__|#elif defined\(__APPLE__\)' port_unix/paraos_timer.hpp
   ```
   Expected: empty output.

---

## Artifacts this phase produces

### Files modified
- `port_unix/paraos_timer.hpp`
- `port_tests/test_timer.cpp`

### Symbols / behavior introduced
- Private member `std::atomic<bool> is_in_run_{false}` to detect when `stop()` is called from inside `run()`.
- Private member `bool period_changed_{false}` to prevent double deadline advance after `change_period()` or repeated `start()`.
- Hardened `start()` that does not recreate an existing worker and marks `period_changed_`.
- Hardened `stop()` that skips worker destruction when invoked from within `run()`.
- Hardened `change_period()` that marks `period_changed_` and wakes the worker.
- Worker-loop logic that handles `period_changed_` after `run()`.

### Build / CLI surface
- `cmake --build build/pc_debug_clang`
- `cmake --build build/pc_debug_gcc`
- `cmake --build build/pc_debug_gcc_clang_tidy`
- `ctest --test-dir build/pc_debug_clang --output-on-failure --stop-on-failure`
- `ctest --test-dir build/pc_debug_gcc --output-on-failure --stop-on-failure`
