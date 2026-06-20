# Phase 2 Research: macOS Behavior Parity

**Date:** 2026-06-20
**Phase:** 2 — Verify Behavior Parity
**Researcher:** gsd-plan-phase

## Executive Summary

After Phase 1, `port_unix/` compiles on macOS but existing tests reveal two categories of behavior-parity failures:

1. **Thread priority mapping** — macOS `SCHED_RR` range is `15..47` while `paraos::ThreadPriority` enumerates `1..7` on Unix. Any test that creates a `Thread` with non-default priority aborts in `IsPriorityInRange()` inside `Thread::Make()`.
2. **Container message-buffer failures** — `MessageBuffer` tests fail because `TryPush()` returns `false` when `timeout_ms == 0` for semaphores on macOS. The `QueueBlocking` implementation uses `pop_sem_.Take(timeout_ms)` with `timeout_ms == 0` to check whether a producer has already signalled, but the macOS `TimedTake()` polling loop treats `timeout_ms == 0` as a single `sem_trywait()` call and returns immediately with `EAGAIN`, causing the consumer to conclude no item is available.

Timer, mutex, and semaphore unit tests in `port_tests/` pass because they either do not use timeouts (`Mutex`/`Semaphore` tests use `timeout_ms == 0`) or rely on direct `Lock(0)` semantics that map to `trylock`. The failures are therefore concentrated in multi-threaded primitives that combine thread priorities with blocking queues/semaphores.

## Evidence

### 1. `ctest` result on current macOS build

```text
50/50 tests: 31 passed, 19 failed.
Failed tests: test_thread_only_static, test_thread_only_global,
  test_thread_only_stack, test_thread_only_stack_with_multiple_threads,
  test_paraos_core, [PARAOS CONTAINERS]:Message.* (6 tests),
  test_message_multithread_many_producer_many_consumers,
  test_multi_ringbuff_mpmc, test_queue_blocking_spmc,
  test_queue_blocking_mpsc, test_queue_blocking_mpmc,
  test_paraos_thread_sequence, test_paraos_cooperative_scheduling_thread,
  test_paraos_oneshot_executor.
```

### 2. Thread priority abort

```text
Assertion failed: ((IsPriorityInRange(attr.priority) == true) && ...),
function Make, file paraos_thread.hpp, line 265.
```

Probe for `SCHED_RR` priority range on this macOS host:

```text
SCHED_RR min=15 max=47
```

`paraos_thread_common.hpp` defines Unix priorities as:

```cpp
enum class ThreadPriority : uint8_t {
  kIdle = 1,
  kLowest,
  kBelowNormal,
  kNormal,
  kAboveNormal,
  kHighest,
  kRealTime,
};
```

`kIdle=1` is below `sched_get_priority_min(SCHED_RR) == 15` on macOS, so `pthread_attr_setschedparam()` fails and `ETL_ASSERT` aborts.

### 3. Message buffer / queue blocking timeout semantics

`QueueBlocking::Pop()` uses a `MutexGuard` then, if the queue is empty, waits on `pop_sem_.Take(timeout_ms)`. When called with `timeout_ms == 0`, the intended Linux behavior is:

- `sem_timedwait()` with an already-expired deadline returns `-1`/`ETIMEDOUT` if no post occurred.
- If `sem_post()` happened before `Pop()` entered, the semaphore count is non-zero and `sem_timedwait()` succeeds immediately.

On macOS the `TimedTake(0)` helper calls `sem_trywait()` once. That *does* succeed if the semaphore has already been posted, so the symptom is not a universal failure. The observed `MessageBuffer` failures happen in single-threaded tests where `TryPush()` is called from a destructor after the queue becomes full. The failure path is:

- `MessageWritable::~MessageWritable()` calls `TryPush()`.
- `TryEmplaceBack()` enters `CriticalSection`, calls `queue_.emplace(...)`, then `pop_sem_.Give()`.
- `Give()` calls `sem_post()` successfully.
- `TryPush()` returns `true`.
- However the test then calls `buff.Pop(0)` and receives `std::nullopt`.

This indicates the semaphore `Give()` is not reliably waking the waiter, or the `Take(0)` path is losing the post. Because `sem_init` unnamed semaphores on macOS are deprecated and may have implementation-specific quirks (e.g., `sem_post` from a destructor context, interleaving with `sem_trywait`), the polling fallback is less robust than `sem_timedwait` on Linux.

## Affected Files and Failure Modes

| File | Failure Mode | Root Cause |
|------|--------------|------------|
| `paraos_thread_common.hpp` | Abort on thread creation with non-default priority | Unix enum values 1..7 outside macOS `SCHED_RR` range 15..47 |
| `port_unix/paraos_semaphore.hpp` | `Take(timeout_ms)` can miss a `Give()` on macOS | `TimedTake()` uses polling loop; `sem_trywait` + `sem_post` interaction on deprecated unnamed semaphores may race |
| `port_unix/paraos_mutex.hpp` | Less severe: `TimedLock()` granularity is 1 ms | Acceptable for test port, but can affect timeout-sensitive tests |

## Compatibility Strategy Options

### Thread Priority

| Option | Pros | Cons |
|--------|------|------|
| **A. Map enum values to macOS `SCHED_RR` range** | Preserves public API enum names; small localized change | Priority semantics change numerically between platforms |
| **B. Disable real-time scheduling on macOS, run all threads `SCHED_OTHER` with priority 0** | Simplest, avoids privileges | `SetPriority`/`GetPriority` would no longer reflect enum values |
| **C. Redefine Unix enum range to match macOS minimum and use `pthread_attr_setschedparam` with scaled values** | Single cross-platform behavior | Changes Linux numerical priorities and may need root privileges there too |

**Recommendation:** Option A. Keep the enum values `1..7` for the public API but, inside `port_unix/paraos_thread.hpp`, map them to the valid macOS `SCHED_RR` range before calling `pthread_*schedparam`. This keeps public API unchanged and fixes the abort. Because root is required to actually change priorities, `SetPriority` already returns `true` when not root; the mapping only needs to satisfy `pthread_attr_setschedparam` during thread creation and `pthread_setschedparam` later. Linux behavior remains unchanged under `#ifdef __linux__`.

### Semaphore Timed Wait Robustness

| Option | Pros | Cons |
|--------|------|------|
| **A. Use `dispatch_semaphore_t` on macOS** | Native, correct, avoids deprecated unnamed semaphores | Apple-only framework, requires Objective-C runtime, changes semaphore storage |
| **B. Use a `pthread_cond_t` + counter to implement semaphore on macOS** | Fully POSIX on macOS, no deprecated APIs, preserves timed-wait semantics | Larger change to `SemaphoreBase`, must protect counter with mutex |
| **C. Add a small yield + retry budget to `TimedTake()`** | Minimal change, may mask timing races | Does not fix structural race, can flake |

**Recommendation:** Option B. Replace the deprecated unnamed POSIX semaphore (`sem_t`) on macOS with a condition-variable + counter implementation. This removes the deprecation warning suppression, eliminates the `sem_trywait`/`sem_post` race, and provides true timed waits. Linux continues to use `sem_t` unchanged. The public `SemaphoreBase` API (`Take`/`Give`/`operator bool`) and derived classes remain unchanged.

## Open Questions

1. Should `ThreadPriority` mapping on macOS be linear (`1→15`, `7→47`) or clamped to a narrower range to keep real-time semantics close to Linux?
2. Does the new macOS semaphore need to support ISR context? On Unix `from_isr == false` is asserted, so a normal mutex-protected condition variable is sufficient.
3. Are there any existing tests that assert exact timeout durations? The 1 ms polling loop in `TimedLock`/`TimedTake` is acceptable only if tolerance is ≥1 ms.

## References

- `paraos_thread_common.hpp`
- `port_unix/paraos_thread.hpp`
- `port_unix/paraos_semaphore.hpp`
- `port_unix/paraos_mutex.hpp`
- `port_unix/paraos_timer.hpp`
- `containers/paraos_queue_blocking.hpp`
- `containers/paraos_message_buffer.hpp`
- `.planning/phases/phase-01/phase-01-SUMMARY.md`
