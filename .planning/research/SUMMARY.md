# Project Research Summary

**Project:** PARAOS — Milestone v1.10 Unix Timer Refactor
**Domain:** Modernizing `port_unix/paraos_timer.hpp` to use internal PARAOS primitives
**Researched:** 2026-06-25
**Confidence:** HIGH

## Executive Summary

The current `port_unix/paraos_timer.hpp` is split between a Linux POSIX timer path (`timer_create`/`timer_settime`/`timer_delete`) and a macOS pthread path (`pthread_mutex_*`, `pthread_cond_*`, `pthread_create`/`pthread_join`). This duplication blocks macOS builds, hides a Linux one-shot timing bug, and forces the project to maintain two platform-specific implementations for a single OSAL abstraction. The v1.10 milestone replaces both paths with one implementation built from existing PARAOS primitives: a `paraos::jthread` worker, a `paraos::mutex` for shared state, and a `paraos::binary_semaphore` for interruptible waits.

Research shows that the recommended primitives are already validated across PC, Unix, and FreeRTOS, so no new dependencies are required. The refactor also dogfoods the same abstractions PARAOS consumers use, removes raw POSIX surface area, and unifies Linux and macOS behavior. The main risks are not in the stack choice but in preserving exact semantics of the existing public API: periodic and one-shot modes, `reset()` re-baselining, `change_period()` waking a sleeping worker, and safe stop/join from both user code and the destructor.

The top mitigations are: use a deadline-based loop with `std::chrono::steady_clock` to avoid accumulated drift; guard `period_ms_` and stop state with `paraos::mutex`; use a `binary_semaphore` only as an early-wake signal; keep the worker in `std::optional<paraos::jthread>` so it exists only while running; and explicitly stop/join in the destructor. A standalone `port_tests/test_timer.cpp` must exercise scheduler gating, because `paraos::jthread` workers block until `start_scheduler()` is called.

## Key Findings

### Recommended Stack

The refactor needs only primitives already present in the PARAOS source tree. No third-party libraries or OS-specific APIs are required.

**Core technologies:**

- **`paraos::jthread`** — replaces `pthread_create`/`pthread_join` and the POSIX `timer_create` callback thread. It provides RAII join, a `stop_token`, and scheduler-gate integration already used by `extra/` helpers.
- **`paraos::binary_semaphore`** — replaces `pthread_cond_signal`/`pthread_cond_timedwait`. `try_acquire_for()` gives an interruptible timed wait that works on Linux, macOS, and FreeRTOS.
- **`paraos::mutex`** — replaces `pthread_mutex_t` for protecting timer state. It is compatible with `std::scoped_lock` and is the project-wide locking primitive.
- **`std::chrono` + `std::chrono::steady_clock`** — expresses periods and deadlines. Required for `try_acquire_for()` and mandatory for a drift-free deadline loop.
- **`std::optional<paraos::jthread>`** — defers thread creation until `start()`, simplifies `stop()`, and supports repeated start/stop cycles.

`paraos::sleep_for` is available as a fallback, but it must not be the primary wait primitive because it is not interruptible. Raw `std::thread`, `timer_create`, and pthread APIs are explicitly out of scope.

### Expected Features

**Must have (table stakes):**

- Public `paraos::timer` API and constructor signature preserved exactly (`period_ms`, `start_immediately`, `is_auto_reload`, `name`; methods `start()`, `stop()`, `reset()`, `change_period()`; virtual `run()`).
- Periodic and one-shot modes work as before.
- `start_immediately` flag creates a timer that begins counting down after construction.
- `start()` on an already-running timer re-evaluates the expiry relative to now.
- `stop()` leaves no running callback; destructor joins the worker.
- `isr_bool` return semantics remain (`true` on success, `false` on failure).
- `max_block_time` and `is_isr` parameters are kept but ignored on Unix for FreeRTOS API compatibility.
- Class stays non-copyable and non-movable.
- Windows and FreeRTOS ports remain untouched.

**Should have (differentiators):**

- Single Linux + macOS code path with no `#ifdef __linux__` / `__APPLE__` branches.
- Correct Linux one-shot delay aligned with macOS/Windows.
- Optional forwarding of the `name` argument to `paraos::thread_attr` for debug visibility.

**Defer (v2+):**

- Shared timer thread pool.
- Public pause/resume or per-start offset APIs.
- New callback/delegate API beyond virtual `run()`.

### Architecture Approach

The new Unix timer is a thin periodic worker built entirely from existing PARAOS primitives.

**Major components:**

1. **`std::optional<paraos::jthread>` worker** — hosts the timer loop and auto-joins on destruction. The thread is emplaced in `start()` and reset in `stop()`.
2. **`paraos::mutex`** — protects mutable state (`period_ms_`, stop/running flags).
3. **`paraos::binary_semaphore`** — acts as an early-wake event for `change_period()`, `reset()`, and `stop()`.
4. **Virtual `run()` callback** — user extension point; must never be called while holding `mutex_`.
5. **Scheduler gate** — workers wait for `paraos::jthread::start_scheduler()` like other PARAOS threads, matching FreeRTOS semantics.

Modified files: `port_unix/paraos_timer.hpp` and `port_tests/CMakeLists.txt`. New file: `port_tests/test_timer.cpp`. Windows and FreeRTOS timer headers stay unchanged.

### Critical Pitfalls

1. **Accumulated timing drift** — replacing the kernel timer with a naive `sleep_for(period)` loop adds execution and scheduling jitter each period. Use a deadline-based loop with `std::chrono::steady_clock` and sleep only the remaining time.
2. **One-shot vs periodic semantics confusion** — a semaphore loop naturally repeats. Branch on `is_auto_reload_` after each callback; do not conflate "one-shot done" with "stop requested".
3. **Stop/join race and destructor deadlock** — calling `stop()` from inside `run()` can make the worker join itself. Detect self-join via thread id, use `std::optional<paraos::jthread>`, and explicitly stop/join in `~timer()`.
4. **Period change while the worker is sleeping** — updating `period_ms_` is not enough; the worker is blocked. Release the semaphore to wake it, then recompute the deadline from "now".
5. **Semaphore overflow or lost wakeups** — repeated `release()` calls on a binary semaphore can throw. Drain pending signals before each wait, or guard wakeups with a mutex-protected flag.
6. **Scheduler-gating surprise** — `paraos::jthread` workers do not run until `start_scheduler()` is called. Document this and ensure `test_timer.cpp` starts the scheduler before expecting callbacks.
7. **clang-tidy failures** — the project treats all tidy warnings as errors. Use explicit lambda return types, `[[nodiscard]]`, `PARAOS_ATTR_UNUSED_VAR`, and narrow `NOLINT` suppressions with rationale.

## Implications for Roadmap

### Phase 1: Design and test scaffold

**Rationale:** The public contract and the available primitives are already well understood, so the first step is to set up a verifiable skeleton.
**Delivers:** Updated `port_unix/paraos_timer.hpp` structure with new includes and private members; new `port_tests/test_timer.cpp` skeleton; `port_tests/CMakeLists.txt` target.
**Addresses:** Preserved public API, RAII worker lifetime.
**Avoids:** Architectural drift by freezing the component boundaries before implementation.

### Phase 2: Core jthread-based loop

**Rationale:** The worker loop is the foundation; everything else builds on it.
**Delivers:** `std::optional<paraos::jthread>` emplaced in `start()`, deadline-based periodic/one-shot loop using `binary_semaphore::try_acquire_for()`, virtual `run()` invocation without holding `mutex_`.
**Uses:** `paraos::jthread`, `paraos::binary_semaphore`, `paraos::mutex`, `std::chrono::steady_clock`.
**Implements:** Worker component and data flow.
**Avoids:** Accumulated drift, one-shot/periodic confusion, non-monotonic clock usage.

### Phase 3: Start/stop/reset/change_period synchronization

**Rationale:** Public methods mutate state that the worker reads concurrently.
**Delivers:** Correct `stop()`, `reset()`, and `change_period()` with early wake, self-join detection, and destructor safety; mutex-protected stop flag; semaphore draining or flag-guarded release.
**Addresses:** `start()` re-arm semantics, period adaptation, reset re-baselining.
**Avoids:** Stop/join deadlock, period-change latency, lost semaphore wakeups, virtual `run()` on a partially destroyed object.

### Phase 4: Standalone test and documentation

**Rationale:** Tests validate behavior and document the scheduler-gating contract.
**Delivers:** Passing `test_timer.cpp` covering periodic/one-shot modes, `change_period()`, `reset()`, `stop()` from `run()`, and destructor safety; code comments explaining scheduler gate behavior.
**Addresses:** New test target, debug visibility of timer threads.
**Avoids:** Scheduler-gating surprise, undetected one-shot/periodic bugs.

### Phase 5: Static analysis and cross-platform regression

**Rationale:** The project fails CI on tidy warnings and relies on multiple presets.
**Delivers:** `pc_debug_clang`, `pc_debug_gcc`, and `*_clang_tidy` presets pass; FreeRTOS presets compile unchanged; Windows port untouched.
**Avoids:** clang-tidy failures, cross-port regressions.

### Phase Ordering Rationale

- The loop must exist before synchronization logic can be exercised.
- Synchronization correctness must be proven before expanding the test suite, or tests will produce false negatives.
- Static analysis and cross-port regression come last because they verify the complete change, not guide design.
- This grouping isolates platform-specific risk to one file and keeps the FreeRTOS/Windows ports untouched.

### Research Flags

Phases likely needing deeper research during planning:

- **Phase 3:** Exact interaction between `paraos::jthread::request_stop()`, the scheduler gate, and a blocked `binary_semaphore::try_acquire_for()` should be validated with a small spike.
- **Phase 3:** Whether `paraos::binary_semaphore` throws on overflow and how the wrapper behaves under rapid `release()` calls needs a runtime check; mitigation strategy (drain vs. flag) depends on the result.

Phases with standard patterns (skip research-phase):

- **Phase 1:** CMake test registration pattern is already established in `port_tests/CMakeLists.txt`.
- **Phase 5:** clang-tidy and preset validation follow the existing CI playbook.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All primitives are shipped in-tree and validated by existing tests. |
| Features | HIGH | The public API is fixed by existing ports and downstream usage. |
| Architecture | HIGH | The worker + mutex + semaphore pattern is already used by `extra/` helpers. |
| Pitfalls | HIGH | Risks are known from the current POSIX/pthread implementation and standard concurrency patterns. |

**Overall confidence:** HIGH

### Gaps to Address

- **Stop-token / scheduler-gate interaction:** Confirm that `request_stop()` followed by `signal_.release()` reliably wakes a worker blocked on the scheduler gate or on `try_acquire_for()`. Handle during Phase 3 implementation.
- **Binary-semaphore overflow semantics:** Verify whether repeated `release()` calls throw and choose the mitigation (drain loop or flag) accordingly. Handle during Phase 3.
- **macOS build host availability:** Validate the refactored header on macOS (or a macOS cross target) because the milestone goal is macOS compatibility; Linux-only CI will not catch macOS-specific issues.

## Sources

### Primary (HIGH confidence)

- `.planning/research/STACK.md` — recommended internal primitives and alternatives.
- `.planning/research/FEATURES.md` — table-stakes features, MVP definition, and priority matrix.
- `.planning/research/ARCHITECTURE.md` — proposed component structure, data flow, and anti-patterns.
- `.planning/research/PITFALLS.md` — critical pitfalls, recovery strategies, and pitfall-to-phase mapping.

### Secondary (MEDIUM confidence)

- `port_unix/paraos_timer.hpp` — current Linux/macOS dual-branch implementation to be replaced.
- `port_freertos/paraos_timer.hpp` — reference for preserved public API semantics.
- `port_win/paraos_timer.hpp` — reference for one-shot vs periodic handling.
- `port_pc/paraos_jthread.hpp`, `port_pc/paraos_mutex_std.hpp`, `port_pc/paraos_semaphore_std.hpp` — primitive APIs and behavior.
- `paraos_sleep.hpp` — cross-platform sleep abstraction.
- `.planning/PROJECT.md` — milestone v1.10 goal and constraints.

---
*Research completed: 2026-06-25*
*Ready for roadmap: yes*
