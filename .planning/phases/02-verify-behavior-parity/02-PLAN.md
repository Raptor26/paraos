---
phase: 2
phase_name: Verify Behavior Parity
plan_id: 02-01
plan_name: Fix macOS Thread Priority and Semaphore Parity
wave: 1
depends_on: []
requirements:
  - PAR-01
  - PAR-02
files_modified:
  - port_unix/paraos_thread.hpp
  - port_unix/paraos_semaphore.hpp
autonomous: true
gap_closure: false
---

# Plan 02-01: Fix macOS Thread Priority and Semaphore Parity

**Owner:** Implementer
**Estimated effort:** Medium
**Input:** Phase 1 build artifacts, `port_tests/`, `containers/tests/`, current `ctest` failures on macOS
**Output:** `pc_debug_clang` CTest suite passes on macOS with behavior identical to Linux

## Objective

Make the public OSAL primitives in `port_unix/` behave identically on macOS and Linux:
- `paraos::Thread` must create threads with valid `SCHED_RR` priorities on macOS without changing the public `ThreadPriority` enum.
- `paraos::SemaphoreBase::Take(timeout_ms)` must reliably receive `Give()` posts on macOS, fixing `QueueBlocking`/`MessageBuffer` consumers.
- All existing tests must pass.

## must_haves

1. `ThreadPriority` enum in `paraos_thread_common.hpp` remains unchanged.
2. `Thread::Make` and `Thread::SetPriority` map enum values to the macOS `SCHED_RR` valid range internally.
3. `SemaphoreBase` API (`Take`, `Give`, `operator bool`, move semantics) remains unchanged.
4. macOS semaphore backend replaces deprecated unnamed `sem_t` with a `pthread_cond_t` + counter implementation.
5. Linux paths remain unchanged and continue to use native `sem_t` and raw enum priorities.
6. All files outside `port_unix/` remain unmodified except for existing Phase 1 minimal changes.
7. `cmake --build build/pc_debug_clang/` and `ctest --test-dir build/pc_debug_clang/` pass with standard CI flags.

## Tasks

### Task 1 — Add macOS thread-priority mapping in `port_unix/paraos_thread.hpp`

<task type="auto">
  <read_first>
    - port_unix/paraos_thread.hpp
    - paraos_thread_common.hpp
  </read_first>
  <action>
    Under `#ifdef __APPLE__`, add a private static helper `MapPriorityToSchedRange(paraos::ThreadPriority)` that linearly maps the public enum values 1..7 into the macOS SCHED_RR range [sched_get_priority_min(SCHED_RR), sched_get_priority_max(SCHED_RR)]. Use `static_cast<int>(priority)` as the source value. Call this helper in `Make()` when passing the priority to `pthread_attr_setschedparam`, in `SetPriority()` when calling `pthread_setschedparam`, and in `IsPriorityInRange()` so the pre-flight check uses the mapped value on macOS. Keep the Linux path exactly as-is under `#ifdef __linux__`.
  </action>
  <acceptance_criteria>
    - `port_unix/paraos_thread.hpp` contains an `#ifdef __APPLE__` `MapPriorityToSchedRange` helper and no public enum changes.
    - `grep -n "MapPriorityToSchedRange" port_unix/paraos_thread.hpp` shows it used in `Make`, `SetPriority`, and `IsPriorityInRange`.
    - `cmake --build build/pc_debug_clang/` succeeds.
    - `ctest --test-dir build/pc_debug_clang -R test_thread_only_static --output-on-failure` passes.
  </acceptance_criteria>
</task>

### Task 2 — Implement a `pthread_cond_t` + counter semaphore backend on macOS

<task type="auto">
  <read_first>
    - port_unix/paraos_semaphore.hpp
    - paraos_thread_common.hpp
  </read_first>
  <action>
    Replace the macOS (`#ifdef __APPLE__`) `sem_t` storage in `SemaphoreBase` with a private `PthreadSemaphore` struct containing `pthread_mutex_t mutex_`, `pthread_cond_t cond_`, `std::size_t count_`, and `std::size_t max_count_`. Initialize these in `SemaphoreCounting` and `SemaphoreBinary` constructors; pass `max_count` and `initial_value` into a new `Create(std::size_t max_count, std::size_t initial_value)` helper on `SemaphoreBase`. Implement `Give()` as `pthread_mutex_lock`; if `count_ < max_count_`, increment `count_` and `pthread_cond_signal`; unlock. Implement `TimedTake(timeout_ms)` as a true condition-variable timed wait using `pthread_cond_timedwait` with `CLOCK_REALTIME` deadline computed from `clock_gettime(CLOCK_REALTIME)`. Treat `timeout_ms == 0` as a single non-blocking check of `count_`; treat `timeout_ms == max_delay` as `pthread_cond_wait` with spurious-wakeup loop. Remove the `#pragma clang diagnostic ignored "-Wdeprecated-declarations"` block because `sem_init`/`sem_destroy` are no longer used on macOS. Keep the Linux path (`sem_t`, `sem_init`, `sem_destroy`, `sem_post`, `sem_timedwait`) exactly as-is under `#ifdef __linux__`.
  </action>
  <acceptance_criteria>
    - `port_unix/paraos_semaphore.hpp` no longer references `sem_t`, `sem_init`, `sem_destroy`, `sem_post`, `sem_trywait`, or `sem_wait` under `#ifdef __APPLE__`.
    - `grep -n "PthreadSemaphore" port_unix/paraos_semaphore.hpp` shows the struct and its members.
    - `cmake --build build/pc_debug_clang/` succeeds with no deprecation warnings.
    - `ctest --test-dir build/pc_debug_clang -R "Message" --output-on-failure` passes all six `[PARAOS CONTAINERS]:Message.*` tests.
  </acceptance_criteria>
</task>

### Task 3 — Run full CTest suite and fix any remaining parity issues

<task type="auto">
  <read_first>
    - build/pc_debug_clang/CTestTestfile.cmake (to discover test target names)
  </read_first>
  <action>
    Run the full `pc_debug_clang` test suite with standard CI flags:
    `ctest --test-dir build/pc_debug_clang --output-on-failure --stop-on-failure --schedule-random --timeout 20 -j4`.
    If any test fails, diagnose whether the failure is a macOS behavior-parity issue inside `port_unix/` or an unrelated flake. Fix only `port_unix/` parity issues; do not modify test expectations. Re-run until the suite passes or only known, documented non-port issues remain.
  </action>
  <acceptance_criteria>
    - `ctest --test-dir build/pc_debug_clang --output-on-failure --stop-on-failure --schedule-random --timeout 20 -j4` reports `100% tests passed`.
    - Any remaining failures are documented in the plan SUMMARY with a clear explanation if they are out of scope.
  </acceptance_criteria>
</task>

### Task 4 — Verify public API surface is unchanged

<task type="auto">
  <read_first>
    - paraos_thread_common.hpp
    - port_unix/paraos_thread.hpp
    - port_unix/paraos_semaphore.hpp
  </read_first>
  <action>
    Confirm that no public class/method signatures, enum values, or member names changed in `paraos_thread_common.hpp`, `port_unix/paraos_thread.hpp`, or `port_unix/paraos_semaphore.hpp`. Check that `git diff` for this phase only touches `port_unix/` (plus unavoidable Phase 1 files already modified).
  </action>
  <acceptance_criteria>
    - `git diff --name-only HEAD` shows only files inside `port_unix/` (and pre-existing Phase 1 changes).
    - Public method signatures in the three headers above are byte-for-byte identical to the pre-Phase-2 versions.
  </acceptance_criteria>
</task>

## Verification Plan

### Automated / Build

- [ ] `cmake --build build/pc_debug_clang/` compiles with no errors or warnings.
- [ ] `ctest --test-dir build/pc_debug_clang --output-on-failure --stop-on-failure --schedule-random --timeout 20 -j4` passes 100%.

### Static / Diff

- [ ] Linux code paths under `#ifdef __linux__` are unchanged.
- [ ] macOS code paths are isolated under `#ifdef __APPLE__`.
- [ ] No files outside `port_unix/` were modified in Phase 2.
- [ ] Public header signatures are unchanged.

### Manual

- [ ] Review `PthreadSemaphore` for lost-wakeup races (lock around `count_` changes, signal while holding lock).
- [ ] Review thread-priority mapping for off-by-one or overflow when `max == min`.

## Risks and Dependencies

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| `pthread_cond_timedwait` spurious wakeups mishandled | Low | Medium | Loop on `count_ == 0` and recompute remaining deadline |
| Priority mapping changes Linux semantics if guard is wrong | Low | High | Keep Linux path under `#ifdef __linux__`; never map on Linux |
| `SemaphoreBinary::is_given_` flag races with new backend | Low | Medium | It wraps `SemaphoreBase::Take`/`Give` under `CriticalSection`; no change |
| `clang-tidy` flags new macOS condition-variable code | Medium | Medium | Follow project `.clang-tidy`; suppress locally with `// NOLINT` if justified |

## Definition of Done

- [ ] All Phase 2 success criteria are met.
- [ ] `02-CONTEXT.md`, `02-RESEARCH.md`, `02-VALIDATION.md`, and `02-PLAN.md` are committed to `.planning/phases/02-verify-behavior-parity/`.
- [ ] Code changes are committed and the full macOS `pc_debug_clang` CTest suite passes.
- [ ] Linux code paths remain unmodified in behavior.
- [ ] `ROADMAP.md` plan checkbox for `02-01` is marked complete.
- [ ] `STATE.md` is updated to reflect Phase 2 complete and readiness for Phase 3.

## Artifacts this phase produces

- Updated `port_unix/paraos_thread.hpp` with macOS priority mapping helper.
- Updated `port_unix/paraos_semaphore.hpp` with `PthreadSemaphore` backend.
- `.planning/phases/02-verify-behavior-parity/02-SUMMARY.md` after execution.

## Notes for Phase 3

- Phase 3 will configure, build, and test every available CMake preset on this macOS machine.
- If the `pc_debug_clang` preset requires explicit compiler overrides to avoid the Arm toolchain in `PATH`, document that in Phase 3.
