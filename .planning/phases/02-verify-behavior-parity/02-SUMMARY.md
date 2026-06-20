---
phase: phase-02
plan: "02-01"
subsystem: port_unix
tags: [macos, pthread, semaphore, thread-priority, behavior-parity]

requires:
  - phase: phase-01
    provides: macOS-compilable port_unix with pthread timer/mutex/semaphore backends

provides:
  - macOS SCHED_RR priority mapping for paraos::Thread without changing the public ThreadPriority enum
  - pthread_cond_t + counter semaphore backend on macOS replacing deprecated unnamed POSIX semaphores
  - 100% passing pc_debug_clang CTest suite on macOS

affects:
  - phase-03
  - phase-04

tech-stack:
  added: []
  patterns:
    - "Internal platform mapping: keep public enum unchanged, map to OS scheduler range inside port_unix"
    - "POSIX condition-variable based counting semaphore for macOS with lost-wakeup-safe signal-while-holding-lock"

key-files:
  created: []
  modified:
    - port_unix/paraos_thread.hpp
    - port_unix/paraos_semaphore.hpp

key-decisions:
  - "Map ThreadPriority enum values 1..7 linearly into macOS SCHED_RR range [15, 47] inside Thread::Make/SetPriority/IsPriorityInRange."
  - "Replace sem_t on macOS with PthreadSemaphore (pthread_mutex_t + pthread_cond_t + count + max_count) to fix lost posts in QueueBlocking/MessageBuffer."
  - "Keep Linux code paths untouched under #ifdef __linux__."

patterns-established:
  - "macOS-only helpers live under #ifdef __APPLE__ and never alter public API signatures."

requirements-completed:
  - PAR-01
  - PAR-02

# Metrics
duration: 20min
completed: 2026-06-20
---

# Phase 2: Verify Behavior Parity Summary

**Fixed macOS runtime behavior parity by mapping thread priorities to the valid SCHED_RR range and replacing deprecated unnamed POSIX semaphores with a pthread condition-variable backend.**

## Performance

- **Duration:** 20 min
- **Started:** 2026-06-20T12:28:00Z
- **Completed:** 2026-06-20T12:48:00Z
- **Tasks:** 4
- **Files modified:** 2

## Accomplishments

- Added `MapPriorityToSchedRange()` helper in `port_unix/paraos_thread.hpp` under `#ifdef __APPLE__` and used it in `Make()`, `SetPriority()`, and `IsPriorityInRange()`, eliminating thread-creation aborts caused by enum values 1..7 falling below macOS `SCHED_RR` minimum (15).
- Reimplemented the macOS semaphore backend in `port_unix/paraos_semaphore.hpp` as `PthreadSemaphore` with mutex-protected count and condition-variable signalling, removing the `-Wdeprecated-declarations` suppression and fixing the lost-post behavior that caused `MessageBuffer`/`QueueBlocking` failures.
- Verified the full `pc_debug_clang` CTest suite passes on macOS: 50/50 tests passed.
- Confirmed no public API signatures changed and all edits remain inside `port_unix/`.

## Task Commits

1. **Task 1: macOS thread-priority mapping** — `6bfcc25` (fix(phase-02): map ThreadPriority to macOS SCHED_RR range)
2. **Task 2: pthread_cond + counter semaphore backend** — `4aba0fa` (fix(phase-02): replace deprecated sem_t on macOS with pthread_cond + counter)
3. **Task 3: Full CTest suite verification** — no code change; verified 100% pass rate
4. **Task 4: Public API surface audit** — no changes required

**Plan metadata:** `07a98e8` (docs(phase-02): context, research, validation, and plan for behavior parity)

## Files Created/Modified

- `port_unix/paraos_thread.hpp` — Added `#ifdef __APPLE__` `MapPriorityToSchedRange()` helper and wired it into thread creation, priority setting, and range validation.
- `port_unix/paraos_semaphore.hpp` — Replaced macOS `sem_t` backend with `PthreadSemaphore` (mutex + condition variable + counter); removed deprecation-warning pragma.

## Decisions Made

- Kept the public `ThreadPriority` enum unchanged; mapping is purely internal to `port_unix/paraos_thread.hpp`.
- Chose a condition-variable semaphore over `dispatch_semaphore_t` to stay within POSIX APIs and preserve the existing `SemaphoreBase` storage/move semantics.
- Did not change `MutexBase::TimedLock()` because its 1 ms polling behavior is acceptable for the test port and is not implicated in current failures.

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered

- Initial `ctest` run after Phase 1 showed 19 failures: thread-priority aborts and message-buffer/queue-blocking semaphore races. Both root causes were isolated to `port_unix/` and fixed without touching tests or public APIs.

## User Setup Required

None.

## Next Phase Readiness

- Phase 2 behavior parity is verified.
- Phase 3 can now configure, build, and test every available CMake preset on this macOS machine.

---
*Phase: phase-02*
*Completed: 2026-06-20*
