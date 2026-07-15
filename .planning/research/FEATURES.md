# Research: Features — Unix `paraos::timer` refactor to internal primitives

**Milestone:** v1.10 Modernize Unix timer with paraos primitives  
**Domain:** Refactoring the existing `port_unix/paraos_timer.hpp` implementation  
**Researched:** 2026-06-25  
**Confidence:** HIGH

## Table Stakes (Essential to Preserve)

Missing these breaks the existing `paraos::timer` contract or downstream code.

| Feature | Why Preserved | Complexity | Notes |
|---------|---------------|------------|-------|
| Public API unchanged | Backward compatibility with user-derived timers | LOW | Constructor signature `(period_ms, start_immediately, is_auto_reload, name)`, methods `start()`, `stop()`, `reset()`, `change_period()`, virtual `run()`, and deprecated aliases must stay identical. |
| Periodic mode (`is_auto_reload == true`) | Core timer use-case | LOW | `run()` must fire repeatedly, spaced by `period_ms`. |
| One-shot mode (`is_auto_reload == false`) | Core timer use-case | LOW | `run()` must fire once after `period_ms`, then the timer becomes dormant. |
| `start_immediately` constructor flag | Existing users rely on auto-start | LOW | When `true`, the timer must begin counting down immediately after construction. |
| `start()` on an already-running timer re-evaluates expiry | `change_period()` and `reset()` delegate to `start()` | LOW | The worker must recompute its next deadline relative to the most recent `start()` call. |
| `stop()` leaves no running callback | Guarantees no `run()` after `stop()` returns | MEDIUM | Must signal the worker and join it before returning. |
| RAII stop/join in destructor | Prevents use-after-free and leaked worker threads | LOW | `paraos::jthread` destructor handles join automatically. |
| `isr_bool` return semantics | Existing callers check success via `operator bool` | LOW | Return `true` on success, `false` on failure (e.g. worker creation failed). |
| `max_block_time` and `is_isr` ignored on Unix | Backward-compatibility params for FreeRTOS API | LOW | Keep them as unused parameters; do not add ISR semantics to Unix. |
| Non-copyable, non-movable | Five-rule contract | LOW | Copy/move constructors and assignments remain deleted. |
| No concurrent `run()` invocations | Current macOS path serializes with one worker; FreeRTOS timer task also serializes | LOW | A single `paraos::jthread` worker naturally enforces this. |
| Windows and FreeRTOS ports untouched | Platform safety requirement | LOW | Refactor is confined to `port_unix/paraos_timer.hpp`. |

## Differentiators / Improvable Behaviors

These are opportunities the refactor creates, not strict preservation requirements.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Unified Linux + macOS implementation | Removes `#ifdef __linux__` / `__APPLE__` branches in the timer | MEDIUM | One code path over `paraos::jthread`, `paraos::mutex`, and `paraos::binary_semaphore`. |
| Correct one-shot delay on Linux | Current Linux path fires after ~1 ns instead of `period_ms` | LOW | Align Linux one-shot semantics with macOS and Windows. |
| Internal primitive dogfooding | Uses the same PARAOS abstractions consumers use | LOW | Reduces direct POSIX surface and platform-specific code. |
| Graceful worker join on destruction | Better than `timer_delete`, which can leave handler threads running | LOW | `paraos::jthread` destructor requests stop and joins. |
| `name` optionally forwarded to `paraos::thread_attr` | Improves debug visibility of timer threads | LOW | The `name` parameter is currently stored but unused; `jthread` supports `thread_attr`. |
| Deterministic single-thread scheduling | Avoids `SIGEV_THREAD` spawning a fresh thread per expiration | LOW | Prevents stacked/overlapping callback invocations. |

## Anti-Features (Explicitly Excluded)

| Feature | Why Tempting | Why Problematic | Alternative |
|---------|--------------|-----------------|-------------|
| Retain POSIX `timer_create` + `timer_settime` Linux path | "Preserves exact Linux timing" | Defeats the milestone goal; macOS still lacks these APIs. | Delete the branch and use internal primitives. |
| Retain macOS `pthread_mutex`/`pthread_cond`/`pthread_create` path | Already works on macOS | Perpetuates two implementations and raw pthread surface. | Replace with `paraos::jthread` + `paraos::mutex` + `paraos::binary_semaphore`. |
| Add public delegate/callback API | More flexible than virtual `run()` | Breaks existing derived-class usage and expands the API. | Keep virtual `run()` as the extension point. |
| Add ISR support to Unix | The FreeRTOS API has `is_isr` | Unix ports run on hosts, not in ISR context; adds complexity for no consumer. | Continue ignoring `is_isr`. |
| Add pause/resume methods | Finer control than start/stop | Public API change; not required by the milestone. | Use `stop()` + `start()`. |
| Add per-start offset/delay parameter | More flexible scheduling | Public API change; not required. | Keep `period_ms` semantics. |
| Shared timer thread pool | Reduces OS thread count | Adds scheduling/synchronization complexity beyond OSAL scope. | One timer owns one `paraos::jthread`. |

## Feature Dependencies

```
[Unified Unix paraos::timer]
    ├──requires──> [paraos::jthread]
    │                  ├──requires──> [std::jthread / std::thread + std::stop_token]
    │                  └──requires──> [paraos::scheduler (PC)]
    ├──requires──> [paraos::mutex]
    │                  └──requires──> [std::mutex]
    ├──requires──> [paraos::binary_semaphore]
    │                  └──requires──> [std::counting_semaphore]
    └──requires──> [paraos::sleep_for]
                           └──requires──> [std::this_thread::sleep_for]

[paraos::stop_token] ──enhances──> [paraos::jthread worker loop]
```

### Dependency Notes

- **[Unified Unix `paraos::timer`] requires [`paraos::jthread`]:** The timer worker thread must be joinable, stoppable, and integrate with the PC scheduler; `paraos::jthread` is the cross-platform abstraction.
- **[`paraos::jthread`] requires [`paraos::scheduler`]:** On PC, threads block on a gate until `start_scheduler()` is called; the timer worker must respect this.
- **[Unified Unix `paraos::timer`] requires [`paraos::mutex`]:** Guards mutable timer state (`period_ms`, running flag, mode) across the public API and the worker thread.
- **[Unified Unix `paraos::timer`] requires [`paraos::binary_semaphore`]:** Allows the public API to wake the worker early for `stop()`, `reset()`, or `change_period()`.
- **[Unified Unix `paraos::timer`] requires [`paraos::sleep_for`]:** Used by the worker to wait for the next expiration when no early signal arrives.
- **[`paraos::stop_token`] enhances [worker loop]:** Using the token provided by `paraos::jthread` keeps the exit condition idiomatic, although a private stop flag + semaphore is functionally equivalent.

## MVP Definition

### Launch With (v1.10)

Minimum viable refactor — must deliver the milestone goal without scope creep.

- [ ] Unified Linux/macOS timer loop implemented on top of `paraos::jthread`, `paraos::mutex`, `paraos::binary_semaphore`, and `paraos::sleep_for`.
- [ ] Public `paraos::timer` API and `run()` semantics preserved exactly.
- [ ] `start()`, `stop()`, `reset()`, `change_period()`, and `is_auto_reload` modes behave correctly.
- [ ] RAII destructor joins the worker thread.
- [ ] New standalone `port_tests/test_timer.cpp` demonstrating basic usage.
- [ ] PC presets (`pc_debug_clang`, `pc_debug_gcc`, `*_clang_tidy`) pass `ctest` without regressions.
- [ ] FreeRTOS presets continue to compile; `port_freertos/paraos_timer.hpp` is not modified.

### Add After Validation (v1.10.x)

- [ ] Forward the timer `name` to `paraos::thread_attr` for the worker thread — improves thread naming in debuggers.
- [ ] Add negative/failure tests for worker creation failure paths.

### Future Consideration (v2+)

- [ ] Shared timer thread pool to reduce thread count — only if profiling shows a real need.
- [ ] `try_lock`/`timed` style control over `start()`/`stop()` — public API change, needs broader design review.

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Preserve public API | HIGH | LOW | P1 |
| Unified jthread-based worker loop | HIGH | MEDIUM | P1 |
| Correct periodic/one-shot semantics | HIGH | LOW | P1 |
| RAII stop/join on destruction | HIGH | LOW | P1 |
| Standalone `test_timer.cpp` | MEDIUM | LOW | P1 |
| Fix Linux one-shot delay | MEDIUM | LOW | P1 |
| No concurrent `run()` invocations | MEDIUM | LOW | P1 |
| Forward `name` to thread attr | LOW | LOW | P2 |
| Shared timer thread pool | LOW | HIGH | P3 |

**Priority key:**
- P1: Must have for v1.10 launch.
- P2: Should have, add when core is stable.
- P3: Nice to have, defer until justified.

## Competitor / Platform Comparison

| Feature | Current Linux (`timer_create`) | Current macOS (pthread) | New Unix (paraos primitives) |
|---------|--------------------------------|------------------------|------------------------------|
| Worker model | Kernel-spawned thread per expiration | Single pthread worker | Single `paraos::jthread` worker |
| One-shot delay | ~1 ns (current bug) | `period_ms` | `period_ms` |
| Stop guarantees | `timer_delete` only; running handler may continue | Joins thread | Joins via `paraos::jthread` destructor |
| macOS support | Not available | Native | Native via same primitives |
| Code duplication | Linux/macOS split | Linux/macOS split | Single path |

---
*Feature research for: Unix `paraos::timer` refactor to internal primitives*  
*Researched: 2026-06-25*
