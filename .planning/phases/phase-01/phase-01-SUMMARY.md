---
phase: phase-01
plan: "01"
subsystem: port_unix
tags: [macos, posix, pthread, timer, mutex, semaphore]

requires:
  - phase: none
    provides: baseline PARAOS project with Linux-focused port_unix

provides:
  - macOS backend for paraos::Timer using a dedicated pthread + condition variable
  - macOS timed-lock helper for paraos::MutexBase
  - macOS timed-take helper for paraos::SemaphoreBase
  - macOS build of the pc_debug_clang preset with system /usr/bin/clang

affects:
  - phase-02
  - phase-03

tech-stack:
  added: []
  patterns:
    - "Platform isolation with #ifdef __linux__ / #ifdef __APPLE__ / #else #error in port_unix headers"
    - "Polling fallback with 1 ms sleeps for missing timed POSIX primitives"

key-files:
  created: []
  modified:
    - port_unix/paraos_timer.hpp
    - port_unix/paraos_mutex.hpp
    - port_unix/paraos_semaphore.hpp
    - paraos_check.h
    - paraos_runtime_profiler.hpp

key-decisions:
  - "Use a dedicated pthread + pthread_cond_timedwait for the macOS Timer backend to preserve the thread-per-timer model."
  - "Use pthread_mutex_trylock / sem_trywait polling loops with 1 ms sleeps for finite timeouts on macOS."
  - "Add __APPLE__ to paraos_check.h and paraos_runtime_profiler.hpp platform guards because AppleClang does not predefine __unix__."
  - "Suppress -Wdeprecated-declarations for sem_init/sem_destroy on macOS; unnamed POSIX semaphores are deprecated but remain functional."

patterns-established:
  - "Each port_unix header keeps the original Linux path under #ifdef __linux__ and adds a macOS path under #ifdef __APPLE__, with #else #error for unsupported Unix variants."

requirements-completed:
  - MAC-01
  - MAC-02
  - MAC-03

# Metrics
duration: 50min
completed: 2026-06-20
---

# Phase 1: Diagnose and Fix macOS Compilation Summary

**Replaced missing POSIX interval-timer and timed-synchronization APIs with macOS-compatible pthread-based implementations, isolated inside `port_unix/` by `__APPLE__` / `__linux__` guards.**

## Performance

- **Duration:** 50 min
- **Started:** 2026-06-20T08:23:00Z
- **Completed:** 2026-06-20T09:13:00Z
- **Tasks:** 5 (audit + 4 implementation/validation tasks)
- **Files modified:** 5

## Accomplishments

- Audited `port_unix/` and confirmed the only macOS-incompatible symbols were `timer_create`/`timer_settime`/`timer_delete`/`timer_t`/`itimerspec`/`SIGEV_THREAD`, `pthread_mutex_timedlock`, and `sem_timedwait`.
- Implemented a macOS `Timer` backend that spawns a dedicated pthread waiting on a condition variable, preserving one-shot/periodic semantics and clean thread join on destruction.
- Added a macOS `TimedLock()` helper in `MutexBase` that polls `pthread_mutex_trylock` with 1 ms sleeps until the deadline.
- Added a macOS `TimedTake()` helper in `SemaphoreBase` that polls `sem_trywait` with 1 ms sleeps until the deadline.
- Verified that `cmake --preset pc_debug_clang -D CMAKE_C_COMPILER=/usr/bin/clang -D CMAKE_CXX_COMPILER=/usr/bin/clang++` configures and builds all `port_unix/` sources and tests without errors.

## Task Commits

1. **Task 1: Audit `port_unix/` API surface** — documented in `PLAN.md` and `RESEARCH.md`
2. **Task 2: Implement macOS Timer backend** — `0a8bdbb` (fix(phase-01): add macOS-compatible backends for Timer, Mutex and Semaphore in port_unix)
3. **Task 3: Implement macOS mutex timed-lock helper** — `0a8bdbb`
4. **Task 4: Implement macOS semaphore timed-wait helper** — `0a8bdbb`
5. **Task 5: Build validation on macOS** — `0a8bdbb`

**Plan metadata:** `6327960` (docs(phase-01): commit Phase 1 research and plan artifacts)

## Files Created/Modified

- `port_unix/paraos_timer.hpp` — Added `#ifdef __APPLE__` pthread/condition-variable timer backend; Linux path preserved under `#ifdef __linux__`.
- `port_unix/paraos_mutex.hpp` — Added macOS `TimedLock()` helper; Linux path uses `pthread_mutex_timedlock()` unchanged.
- `port_unix/paraos_semaphore.hpp` — Added macOS `TimedTake()` helper and suppressed deprecated unnamed-semaphore warnings; Linux path uses `sem_timedwait()` unchanged.
- `paraos_check.h` — Added `defined(__APPLE__)` to the platform guard so debug assert macros are available on macOS.
- `paraos_runtime_profiler.hpp` — Added `defined(__APPLE__)` to the `OsProfiler`/`OsTimer` platform guard so `paraos_time.hpp` and socket examples compile.

## Decisions Made

- Kept all changes inside `port_unix/` where possible; the only non-port_unix edits were two minimal `__APPLE__` additions in core headers required because AppleClang does not define `__unix__`.
- Chose polling loops with 1 ms sleeps over `dispatch_source_t`/`dispatch_semaphore_t` to avoid Apple-only frameworks and keep the port POSIX-oriented.
- Preserved public class signatures and member storage (`pthread_mutex_t`, `sem_t`) so Linux behavior is unchanged.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] AppleClang does not define `__unix__`, so debug and profiler headers were disabled on macOS**
- **Found during:** Task 5 (Build validation)
- **Issue:** `PARAOS_CHECK_ASSERT`/`PARAOS_CHECK_LOOP` were undefined and `OsProfiler` was missing, causing compilation failures in `port_tests/` and `extra/`.
- **Fix:** Added `defined(__APPLE__)` to the platform guards in `paraos_check.h` and `paraos_runtime_profiler.hpp`.
- **Files modified:** `paraos_check.h`, `paraos_runtime_profiler.hpp`
- **Verification:** `cmake --build build/pc_debug_clang/` compiles all tests and examples.
- **Committed in:** `0a8bdbb`

**2. [Rule 3 - Blocking] macOS deprecates unnamed POSIX semaphores (`sem_init`/`sem_destroy`)**
- **Found during:** Task 5 (Build validation)
- **Issue:** `-Werror` treated deprecation warnings as errors.
- **Fix:** Wrapped the `paraos_semaphore.hpp` body with `#pragma clang diagnostic ignored "-Wdeprecated-declarations"` on `__APPLE__`.
- **Files modified:** `port_unix/paraos_semaphore.hpp`
- **Verification:** `port_tests/test_semaphore.cpp` and dependent targets compile.
- **Committed in:** `0a8bdbb`

---

**Total deviations:** 2 auto-fixed (1 missing critical, 1 blocking)
**Impact on plan:** Both deviations were necessary for a clean macOS build. They are minimal, preserve Linux paths, and do not change public APIs.

## Issues Encountered

- The default `clang` in `PATH` targets `aarch64-unknown-linux-gnu`, so CMake configuration must override the compiler with `/usr/bin/clang` and `/usr/bin/clang++`. This was validated as specified in `RESEARCH.md`.
- A runtime test failure was discovered after the build succeeded: `test_paraos_cooperative_scheduling_thread` aborts in `Thread::Make` because macOS `SCHED_RR` priority range starts at 15, while the `ThreadPriority` enum uses values 1..7. This is a behavior-parity issue outside Phase 1's compilation scope and has been noted for Phase 2.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 1 build artifacts are in place.
- Phase 2 should investigate and fix the macOS thread-priority mapping and then run behavior-parity tests for timers, mutexes, and semaphores.

---
*Phase: phase-01*
*Completed: 2026-06-20*
