# Architecture Research — v1.10 Unix Timer Refactor

**Domain:** PARAOS OSAL — internal restructuring of `port_unix/paraos_timer.hpp`
**Milestone:** v1.10 Modernize Unix timer with paraos primitives
**Researched:** 2026-06-25
**Confidence:** HIGH

## System Overview

The Unix timer becomes a thin periodic worker built entirely from PARAOS primitives already used by `extra/` and container tests. It no longer branches into Linux POSIX timers or macOS pthread condition variables.

```
┌─────────────────────────────────────────────────────────────────┐
│                     Public API (unchanged)                       │
│  paraos::timer  ──start()/stop()/reset()/change_period()───►     │
│                 ──virtual run() ◄───────────────────────────     │
├─────────────────────────────────────────────────────────────────┤
│              New unified Unix implementation                     │
│  ┌─────────────┐  ┌─────────────┐  ┌───────────────────────┐   │
│  │ jthread     │  │ mutex       │  │ binary_semaphore      │   │
│  │ (timer loop)│  │ (state)     │  │ (early-wake signal)   │   │
│  └──────┬──────┘  └──────┬──────┘  └───────────┬───────────┘   │
│         │                │                      │               │
│         └────────────────┴──────────────────────┘               │
│                          │                                       │
│                    optional<jthread>                             │
├─────────────────────────────────────────────────────────────────┤
│                     Existing PARAOS primitives                   │
│  port_unix/paraos_jthread.hpp ──► port_pc/paraos_jthread.hpp     │
│  port_unix/paraos_mutex_std.hpp ──► port_pc/paraos_mutex_std.hpp │
│  port_unix/paraos_semaphore_std.hpp ──► port_pc/...              │
│  paraos_sleep.hpp                                                │
└─────────────────────────────────────────────────────────────────┘
```

## Component Responsibilities

| Component | Responsibility | Implementation in refactored header |
|-----------|----------------|-------------------------------------|
| `paraos::jthread` | Hosts the periodic timer loop; auto-joins on destruction | `std::optional<paraos::jthread> thread_` |
| `paraos::mutex` | Protects mutable timer state (`period_ms_`, `is_stop_requested_`) | `paraos::mutex mutex_` |
| `paraos::binary_semaphore` | Allows `start()`, `stop()`, `reset()`, `change_period()` to wake the sleeping worker early | `paraos::binary_semaphore signal_{0}` |
| `paraos::sleep_for` | Not used directly for waiting; kept as the project-wide sleep primitive for diagnostic/backoff code | Included for consistency |
| `std::optional` | Deferred thread creation so a timer can exist without an OS thread until `start()` | `std::optional<paraos::jthread>` |

## New vs Modified Components

### Modified

| File | What changes |
|------|--------------|
| `port_unix/paraos_timer.hpp` | Replaces Linux `timer_create`/`timer_settime`/`timer_delete` and macOS `pthread_mutex`/`pthread_cond`/`pthread_create` with a single `jthread` + `mutex` + `binary_semaphore` worker. Public API is preserved. |
| `port_tests/CMakeLists.txt` | Adds `test_timer.cpp` target with `cxx_std_20`, warning flags, and optional `CXX_CLANG_TIDY` wiring (same pattern as `test_jthread_basic.cpp`). |

### New

| File | Purpose |
|------|---------|
| `port_tests/test_timer.cpp` | Standalone smoke test demonstrating `paraos::timer` construction, `start()`, periodic callback counting, and clean shutdown via `stop()` / destructor. Follows the `test_jthread_basic.cpp` pattern: calls `paraos::jthread::start_scheduler()`, waits for expected ticks, then exits. |

### Unchanged

| File | Rationale |
|------|-----------|
| `port_win/paraos_timer.hpp` | Windows uses `CreateTimerQueueTimer`; out of scope for v1.10. |
| `port_freertos/paraos_timer.hpp` | FreeRTOS uses `xTimerCreate`; out of scope for v1.10. |
| `port_pc/paraos_jthread.hpp`, `port_pc/paraos_mutex_std.hpp`, `port_pc/paraos_semaphore_std.hpp` | Already provide the primitives needed by the new Unix timer. |

## Proposed Structure for `port_unix/paraos_timer.hpp`

```cpp
#ifndef PARAOS_TIMER_HPP
#define PARAOS_TIMER_HPP

#include <atomic>
#include <chrono>
#include <optional>
#include <string_view>

#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_isr.hpp"
#include "paraos_jthread.hpp"
#include "paraos_mutex_std.hpp"
#include "paraos_semaphore_std.hpp"
#include "paraos_sleep.hpp"
#include "paraos_utils.hpp"

namespace paraos {

class timer {
 public:
  explicit timer(
      std::size_t period_ms, bool start_immediately = false,
      bool is_auto_reload = true, std::string_view name = "Timer")
      : period_ms_{period_ms}, is_auto_reload_{is_auto_reload}, name_{name} {
    if (start_immediately) {
      start();
    }
  }

  virtual ~timer() { stop(); }

  auto start(paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> isr_bool {
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    std::scoped_lock lock{mutex_};
    if (thread_.has_value()) {
      // Already running: wake the worker so it recomputes the deadline.
      signal_.release();
      return isr_bool{true};
    }

    is_stop_requested_ = false;
    thread_.emplace([this](paraos::stop_token token) {
      run_timer_loop(token);
    });
    return isr_bool{true};
  }

  auto change_period(
      std::size_t period_ms, paraos::delay_type max_block_time = max_delay,
      bool is_isr = false) -> isr_bool {
    {
      std::scoped_lock lock{mutex_};
      period_ms_ = period_ms;
    }
    return start(max_block_time, is_isr);
  }

  auto stop(paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> isr_bool {
    PARAOS_ATTR_UNUSED_VAR(max_block_time);
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    {
      std::scoped_lock lock{mutex_};
      is_stop_requested_ = true;
    }
    signal_.release();

    if (thread_.has_value()) {
      thread_->request_stop();
      thread_->join();
      thread_.reset();
    }

    {
      std::scoped_lock lock{mutex_};
      is_stop_requested_ = false;
    }

    return isr_bool{true};
  }

  auto reset(paraos::delay_type max_block_time = max_delay, bool is_isr = false)
      -> isr_bool {
    return start(max_block_time, is_isr);
  }

  virtual void run() {
    PARAOS_CHECK_ASSERT(false);
  }

  // Five rule.
  timer(timer&& other) = delete;
  auto operator=(timer&& other) -> timer& = delete;
  auto operator=(const timer& other) -> timer& = delete;
  timer(const timer& other) = delete;

  // Deprecated CamelCase wrappers unchanged.
  // ...

 private:
  void run_timer_loop(paraos::stop_token token) {
    while (!token.stop_requested()) {
      std::chrono::milliseconds period;
      {
        std::scoped_lock lock{mutex_};
        if (is_stop_requested_) {
          break;
        }
        period = std::chrono::milliseconds(period_ms_);
      }

      // Wait for the period, but allow early wakeup on start/reset/change_period/stop.
      if (signal_.try_acquire_for(period)) {
        // Woken early: recompute the deadline with the current period_ms_.
        continue;
      }

      // Timeout: fire the user callback.
      {
        std::scoped_lock lock{mutex_};
        if (is_stop_requested_ || token.stop_requested()) {
          break;
        }
      }

      run();

      if (!is_auto_reload_) {
        break;
      }
    }
  }

  std::size_t period_ms_;
  bool is_auto_reload_;
  std::string_view name_;

  paraos::mutex mutex_;
  paraos::binary_semaphore signal_{0};
  std::optional<paraos::jthread> thread_;
  bool is_stop_requested_{false};
};

using Timer PARAOS_DEPRECATED("use paraos::timer") = timer;

}  // namespace paraos

#endif /* PARAOS_TIMER_HPP */
```

## Architectural Patterns

### Pattern 1: Optional deferred OS thread

**What:** `std::optional<paraos::jthread>` lets the timer object exist without consuming an OS thread until `start()` is called.

**When to use:** When a resource may be created in a stopped state and later started repeatedly.

**Trade-offs:** Simplifies `stop()` (reset optional) and avoids thread creation in the constructor, but adds a small indirection compared to a raw `jthread` member.

### Pattern 2: Semaphore-as-event for interruptible sleeps

**What:** A `binary_semaphore` with initial count `0` is used as an event. `try_acquire_for(period)` sleeps for `period` unless `release()` is called, which provides the early-wakeup needed for `reset()` and `change_period()`.

**When to use:** Anywhere a worker must sleep with a timeout while still being responsive to control signals.

**Trade-offs:** Replaces non-interruptible `sleep_for()`; one pending release may be consumed by a spurious early loop iteration, but each iteration recomputes the deadline, so behavior stays correct.

### Pattern 3: Scheduler-aware worker threads

**What:** Because `paraos::jthread` gates threads until `start_scheduler()` is called, the timer loop starts only when the scheduler allows. This matches FreeRTOS semantics and the rest of PARAOS.

**When to use:** All PARAOS worker threads (`extra/`, containers tests) already follow this pattern.

**Trade-offs:** Standalone tests must call `paraos::jthread::start_scheduler()`; otherwise the timer thread waits at the gate.

## Data Flow

### `start()` on a stopped timer

```
User code: start()
    ↓
[lock mutex]
    ↓
thread_ empty? ──yes──► is_stop_requested_ = false
    │                       ↓
    │                   emplace jthread
    │                       ↓
    │                   [thread starts, waits at scheduler gate]
    │
    └──no──► signal_.release() (wake to recompute deadline)
    ↓
[unlock mutex]
```

### Timer loop firing `run()`

```
[lock mutex]
    ↓
read period_ms_ + is_stop_requested_
    ↓
[unlock mutex]
    ↓
signal_.try_acquire_for(period_ms_)
    ↓
┌──────────────────┬──────────────────┐
│ timeout          │ release() woke   │
│   ↓              │   early          │
│ run() called     │   ↓              │
│   ↓              │ continue loop    │
│ if !auto_reload  │ (recompute)      │
│   break          │                  │
└──────────────────┴──────────────────┘
```

### `stop()` / destructor

```
User code: stop() or ~timer()
    ↓
[lock mutex] ──► is_stop_requested_ = true
    ↓
[unlock mutex]
    ↓
signal_.release()
    ↓
thread_->request_stop()
thread_->join()
thread_.reset()
    ↓
[lock mutex] ──► is_stop_requested_ = false
    ↓
[unlock mutex]
```

## Build Order

1. **Refactor `port_unix/paraos_timer.hpp`** — remove POSIX/pthread branches, add includes for `paraos_jthread.hpp`, `paraos_mutex_std.hpp`, `paraos_semaphore_std.hpp`, `paraos_sleep.hpp`, and implement the unified loop.
2. **Add `port_tests/test_timer.cpp`** — standalone smoke test that derives from `paraos::timer`, counts `run()` invocations, starts the scheduler, waits for the expected count, and stops cleanly.
3. **Update `port_tests/CMakeLists.txt`** — register `test_timer` as an executable with `cxx_std_20`, warning flags, `TIMEOUT 20`, and optional `CXX_CLANG_TIDY` properties.
4. **Configure and build `pc_debug_clang`** — verify compilation and run `ctest`.
5. **Build `pc_debug_gcc`** and `*_clang_tidy` presets — confirm no new warnings/errors.
6. **Build FreeRTOS presets** — confirm `port_freertos/paraos_timer.hpp` is unaffected.

## Integration Points

| Primitive | Header included by new timer | Provided by | Notes |
|-----------|------------------------------|-------------|-------|
| `paraos::jthread` | `paraos_jthread.hpp` | `port_pc/paraos_jthread.hpp` via `port_unix/paraos_jthread.hpp` | Worker thread; auto-join on `thread_.reset()` handles gated scheduler shutdown. |
| `paraos::mutex` | `paraos_mutex_std.hpp` | `port_pc/paraos_mutex_std.hpp` via `port_unix/paraos_mutex_std.hpp` | Guards `period_ms_` and `is_stop_requested_`. |
| `paraos::binary_semaphore` | `paraos_semaphore_std.hpp` | `port_pc/paraos_semaphore_std.hpp` via `port_unix/paraos_semaphore_std.hpp` | Used as an interruptible event. |
| `paraos::sleep_for` | `paraos_sleep.hpp` | Root header | Included for consistency; not used for the timed wait because it is non-interruptible. |

### Internal boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| `port_unix/paraos_timer.hpp` ↔ `paraos::jthread` | Direct member + lambda callback | The lambda receives `paraos::stop_token` as its last argument. |
| `port_unix/paraos_timer.hpp` ↔ `paraos::scheduler` | Indirect via `jthread` gate | Timer threads wait for `start_scheduler()` like all other PARAOS workers. |
| `port_unix/paraos_timer.hpp` ↔ user `run()` | Virtual callback | Must not hold `mutex_` while calling `run()` to avoid deadlock if `run()` re-enters timer methods. |

## Anti-Patterns

### Anti-Pattern 1: Holding the mutex while calling `run()`

**What people do:** Lock `mutex_`, then call the virtual `run()` inside the critical section.

**Why it's wrong:** User overrides of `run()` may call `start()`, `stop()`, or `change_period()`, which try to acquire the same mutex and deadlock.

**Do this instead:** Release `mutex_` before calling `run()`, then re-acquire only to check `is_stop_requested_` and `is_auto_reload_`.

### Anti-Pattern 2: Using `paraos::sleep_for()` as the timer wait primitive

**What people do:** Replace the POSIX timed wait with `paraos::sleep_for(period)`.

**Why it's wrong:** `std::this_thread::sleep_for` cannot be interrupted, so `stop()`, `reset()`, and `change_period()` would block until the current period ends.

**Do this instead:** Use `paraos::binary_semaphore::try_acquire_for(period)` for an interruptible timeout.

### Anti-Pattern 3: Creating the OS thread in the constructor

**What people do:** Start the worker thread inside the constructor when `start_immediately == false`.

**Why it's wrong:** Wastes an OS thread for a timer that may never be started; inconsistent with the existing FreeRTOS and Windows implementations where `start()` is the explicit creation point.

**Do this instead:** Keep the thread in `std::optional` and emplace it only in `start()`.

## Sources

- `.planning/PROJECT.md` — v1.10 milestone goal and constraints.
- `port_unix/paraos_timer.hpp` — current Linux/macOS dual-branch implementation.
- `port_pc/paraos_jthread.hpp` — `paraos::jthread` API and scheduler gate behavior.
- `port_pc/paraos_mutex_std.hpp` — `paraos::mutex` API.
- `port_pc/paraos_semaphore_std.hpp` — `paraos::binary_semaphore` API.
- `paraos_sleep.hpp` — cross-platform sleep primitive.
- `port_tests/CMakeLists.txt` — existing standalone test registration pattern.

---
*Architecture research for: PARAOS v1.10 Unix timer refactor*
*Researched: 2026-06-25*
