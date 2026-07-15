# Pitfalls Research

**Domain:** Unix timer refactor — replacing POSIX `timer_create`/`pthread_cond`/`pthread_create` in `port_unix/paraos_timer.hpp` with `paraos::jthread`, `paraos::mutex`, `paraos::*_semaphore`, and `paraos::sleep_for`.
**Researched:** 2026-06-25
**Confidence:** HIGH

## Critical Pitfalls

### Pitfall 1: Accumulated timing drift in manual sleep loops

**What goes wrong:**
A timer implemented as `while (running) { sleep_for(period); run(); }` slips because the sleep interval does not account for the time spent executing `run()` or scheduling jitter. Over many periods the drift grows, and the consumer observes intervals longer than the configured `period_ms`. The old Linux path used kernel-managed `timer_settime`, which does not have this drift.

**Why it happens:**
Developers often replace a kernel timer with a simple `sleep_for(period_ms)` loop because it is the direct semantic equivalent of "wait, then fire". Without deadline accounting, each iteration adds the execution time of `run()` and the scheduler latency to the next period.

**How to avoid:**
Use a deadline-based loop with `std::chrono::steady_clock`. Compute the next absolute time point as `previous_deadline + period`, then sleep only the remaining duration. On period change or reset, recompute the deadline relative to "now". Do not use `paraos::sleep_for(period_ms_)` as the only timing primitive.

**Warning signs:**
- Unit tests with many iterations fail intermittently because elapsed time exceeds `N * period_ms`.
- `*_clang_tidy` passes but runtime stress tests show jitter proportional to `run()` duration.

**Phase to address:**
Phase 2 — Core jthread-based loop implementation.

---

### Pitfall 2: One-shot vs periodic semantics confusion

**What goes wrong:**
A one-shot timer (`is_auto_reload == false`) fires repeatedly, or a periodic timer fires only once. The old POSIX path set `it_interval = 0` for one-shot and `it_interval = it_value` for periodic; the macOS path set `is_running = false` after the first callback.

**Why it happens:**
A semaphore-release loop does not naturally distinguish one-shot from periodic. If the worker simply waits, runs, and loops, one-shot timers become periodic. Conversely, if the worker exits after the first run, periodic timers created with `start_immediately = true` and later `change_period` to periodic may stop unexpectedly.

**How to avoid:**
Store `is_auto_reload_` in the worker state and check it after each firing. In one-shot mode, set a local `should_continue = false` and do not re-arm. Use a single loop body that branches on `is_auto_reload_`, and do not conflate "stop requested" with "one-shot done".

**Warning signs:**
- A standalone test expecting a single callback receives two or more.
- A periodic timer stops after the first callback when `change_period()` is called.

**Phase to address:**
Phase 2 — Core jthread-based loop implementation.

---

### Pitfall 3: Stop/join race and destructor deadlock

**What goes wrong:**
`stop()` joins the worker thread. If `stop()` is called from inside `run()`, the worker joins itself and deadlocks. The destructor can also deadlock if `run()` is still executing and holds a lock that `stop()` needs, or if the base class has already begun destruction while the worker calls the soon-to-be-destroyed virtual `run()`.

**Why it happens:**
`paraos::jthread` destructs by requesting stop and joining, but the timer wrapper must coordinate the worker exit safely. The existing macOS path already joins in `stop()` and `~timer()`, so any refactor must preserve the "do not join self" invariant.

**How to avoid:**
- Detect `std::this_thread::get_id() == worker_id_` in `stop()` and return success without joining.
- In the destructor, request stop first, then join; do not call virtual `run()` after the destructor body begins.
- Use `std::optional<paraos::jthread>` so the worker exists only when the timer is running, and destruction naturally joins.
- Keep the worker's exit condition independent of locks that `run()` might take.

**Warning signs:**
- Tests time out at 20 seconds on `stop()` or at process exit.
- ASan/TSan reports data race on the timer object's vtable.

**Phase to address:**
Phase 3 — Start/stop/reset/change_period synchronization.

---

### Pitfall 4: Period change while the worker is sleeping

**What goes wrong:**
Calling `change_period()` updates `period_ms_` but the worker continues sleeping the old duration, then uses the new period only for the next interval. The old POSIX path re-armed the kernel timer atomically; the macOS path signaled the condition variable to wake the worker.

**Why it happens:**
With a sleep-based loop, the worker reads `period_ms_` once before sleeping. A later update is invisible until the sleep returns. A naive atomic store is not enough because the worker is blocked.

**How to avoid:**
Use a `paraos::binary_semaphore` or condition-variable-like primitive to wake the worker when `period_ms_` changes. Recompute the next deadline from the current time using the new period immediately after waking. Protect the shared state with `paraos::mutex` and `std::scoped_lock` / `std::unique_lock`.

**Warning signs:**
- `change_period()` returns true but the next callback still uses the old period.
- Tests for period adaptation fail nondeterministically.

**Phase to address:**
Phase 3 — Start/stop/reset/change_period synchronization.

---

### Pitfall 5: `reset()` does not restart the deadline from "now"

**What goes wrong:**
`reset()` is documented to re-evaluate the expiry time relative to the call moment. A naive implementation that just calls `start()` on an already-running timer may leave the original deadline intact, so the next callback fires at the old time.

**Why it happens:**
The current Unix implementation delegates `reset()` to `start()`. On Linux, `timer_settime` re-arms relative to now. On macOS, the worker path signals the condition variable to recompute the deadline. A new implementation must explicitly reset the baseline.

**How to avoid:**
Treat `reset()` as "wake worker, set deadline = now + period_ms_, keep running". Do not skip the deadline reset if the timer is already active. Reuse the same wake mechanism as `change_period()`.

**Warning signs:**
- A test calls `reset()` immediately before an expected callback and observes the callback at the original time.
- `reset()` on a stopped timer behaves correctly, but `reset()` on a running timer does not.

**Phase to address:**
Phase 3 — Start/stop/reset/change_period synchronization.

---

### Pitfall 6: Calling virtual `run()` on a partially destroyed object

**What goes wrong:**
If the worker is still running when `~timer()` returns, the next call to the virtual `run()` happens after the derived class destructor has run, invoking undefined behavior.

**Why it happens:**
`paraos::jthread` joins on destruction, but only if the wrapper destructor reaches that point. If the timer stores the jthread as a direct member and the worker is not explicitly stopped before base-class destruction begins, the race window exists.

**How to avoid:**
- Stop and join the worker in the `timer` destructor body before any base-class destruction.
- Use `std::optional<paraos::jthread>` and `reset()` it in `stop()` after join, so the worker lifetime is bounded.
- Never rely on implicit destruction order to join; make join explicit in `~timer()`.

**Warning signs:**
- Random crashes or sanitizer reports at program exit for users who create timers with automatic storage.
- Pure virtual call errors in derived classes.

**Phase to address:**
Phase 3 — Start/stop/reset/change_period synchronization.

---

### Pitfall 7: Scheduler-gating surprise with `paraos::jthread`

**What goes wrong:**
On PC platforms, `paraos::jthread` workers block inside `WaitForGate()` until `start_scheduler()` is called. A `paraos::timer` created before the scheduler starts will not fire until the scheduler starts, which differs from the old POSIX timer behavior on Linux.

**Why it happens:**
The refactor replaces the OS timer thread with a `paraos::jthread`, which inherits the scheduler gate from `port_pc/paraos_jthread.hpp`. This is correct for thread primitives but changes timer semantics for applications that create timers before starting the scheduler.

**How to avoid:**
Document the behavior change in code comments and in the standalone test. Do not try to bypass the gate — the uniformity with other paraos threads is the goal of v1.10. Ensure the standalone test starts the scheduler before expecting callbacks.

**Warning signs:**
- The new `test_timer.cpp` hangs at startup if it does not call `paraos::jthread::start_scheduler()`.
- Existing user code on Unix reports timers never firing after the refactor.

**Phase to address:**
Phase 4 — Standalone test and documentation.

---

### Pitfall 8: Semaphore overflow or lost wakeups

**What goes wrong:**
Using `paraos::binary_semaphore` for wakeups, repeated `release()` calls without an intervening `acquire()` can overflow the semaphore's max count and throw `std::runtime_error`. Alternatively, a `release()` before the worker starts waiting can be lost, causing the worker to sleep the full old period.

**Why it happens:**
`paraos::binary_semaphore` is `counting_semaphore<1>` backed by `std::counting_semaphore`. Its `release()` throws on overflow. A spurious second `change_period()` before the worker drains the first signal triggers the exception.

**How to avoid:**
- Drain the semaphore before each wait (`try_acquire()` in a loop) if there is any chance of spurious signals.
- Or use a condition-variable style pattern: hold a mutex, set a "wakeup requested" flag, then release the semaphore. The worker clears the flag under the mutex after waking.
- Do not issue unconditional `release()` calls from `change_period()` without accounting for prior unreleased signals.

**Warning signs:**
- `std::runtime_error("semaphore release overflow")` in tests.
- `change_period()` followed quickly by another `change_period()` deadlocks or throws.

**Phase to address:**
Phase 3 — Start/stop/reset/change_period synchronization.

---

### Pitfall 9: Using non-monotonic clock for deadlines

**What goes wrong:**
If the implementation uses `std::chrono::system_clock` or the old `CLOCK_REALTIME` arithmetic, system time changes (NTP sync, user adjustment) shift timer deadlines, causing missed or premature callbacks.

**Why it happens:**
The old Linux path used `CLOCK_REALTIME` because `timer_create` was created with it. A manual loop does not have that constraint, and developers may copy the old timespec arithmetic out of habit.

**How to avoid:**
Use `std::chrono::steady_clock` for all deadline calculations in the worker loop. Convert `period_ms_` to `std::chrono::milliseconds`. Avoid `timespec_add` and `milliseconds_in_timespec` in the new implementation unless they are needed for an external interface.

**Warning signs:**
- Timer behavior changes when the system clock is adjusted during tests.
- Use of `CLOCK_REALTIME` or `system_clock` appears in the diff for `port_unix/paraos_timer.hpp`.

**Phase to address:**
Phase 2 — Core jthread-based loop implementation.

---

### Pitfall 10: `clang-tidy` failures from new C++ patterns

**What goes wrong:**
The project runs `WarningsAsErrors: '*'` in `.clang-tidy`. New code that looks idiomatic can still fail checks such as `cppcoreguidelines-avoid-non-const-global-variables`, `readability-implicit-bool-conversion`, `modernize-use-auto`, `google-explicit-constructor`, `cppcoreguidelines-pro-type-static-cast-downcast`, or `llvm-prefer-static-over-anonymous-namespace`.

**Why it happens:**
Static analysis is stricter than compilation. Helpers like `static_cast<void*>(this)` passed to a thread function, implicit `bool` conversions of `is_running_`, or lambda captures without explicit return types trigger warnings.

**How to avoid:**
- Prefer `std::optional<paraos::jthread>` over raw `pthread_t`/`timer_t` state.
- Use explicit `static_cast<void>()` only where needed; mark unused parameters with `PARAOS_ATTR_UNUSED_VAR`.
- Give lambdas explicit `-> void` return types when captured types are complex, as was done in `extra/` during v1.8.
- Use `[[nodiscard]]` on read-only methods; mark deprecated forwarders consistently.
- If a check is genuinely impossible to satisfy, document it with inline `// NOLINT(...)` comments and keep the suppression as narrow as `NOLINTBEGIN/NOLINTEND` around the single class or lambda.

**Warning signs:**
- `*_clang_tidy` CMake preset fails after the refactor even though `pc_debug_*` builds.
- Warnings mention new timer code specifically.

**Phase to address:**
Phase 5 — clang-tidy and cross-platform regression.

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Sleep-only loop (`sleep_for(period)`) | Simplest code | Accumulated drift, poor period accuracy | Never — use deadline-based loop |
| Single `binary_semaphore` for both stop and period-change wakeups | Fewer members | Lost wakeups, overflow, ambiguous wake reason | Only if guarded by a state flag under mutex |
| Reuse `period_ms_` as both delay and period without re-baselining | Fewer variables | `reset()` and `change_period()` semantics break | Never |
| Keep `timer_t`/`pthread_t` fields alongside jthread "just in case" | Easier rollback | Dead code, duplicated state, tidy warnings | Never — remove them in the refactor |
| Inline worker lambda with implicit return type | Less typing | clang-tidy warnings and hard-to-read captures | Never — use explicit `-> void` |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| `paraos::jthread` scheduler gate | Creating a timer and expecting it to fire before `start_scheduler()` | Document that timers are jthreads and obey the scheduler gate; start scheduler in tests |
| `paraos::mutex` with `std::unique_lock` | Using `std::condition_variable` directly, which expects `std::mutex` | Use `std::unique_lock<paraos::mutex>` if the mutex wrapper exposes compatibility, or use a semaphore for wakeup |
| `paraos::binary_semaphore` | Treating it like an event flag with unbounded `release()` | Use a flag under mutex plus a single `release()`; drain before wait |
| `paraos::sleep_for` | Using it as the primary timing source | Use it only to sleep the remaining time to a steady-clock deadline |
| FreeRTOS `port_freertos/paraos_timer.hpp` | Changing shared headers or assumptions | Do not touch FreeRTOS port; keep `isr_bool` semantics identical |
| Windows `port_win/paraos_timer.hpp` | Assuming Unix refactor changes Windows behavior | Leave Windows port untouched; preserve cross-platform API only |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Busy-wait on period zero | CPU spin, 100% core usage | Treat `period_ms_ == 0` as a minimum meaningful delay or document it as unsupported | One-shot timers with very small/zero delay |
| High-resolution spin for deadline | CPU burn waiting for exact deadline | Sleep for `deadline - now - margin`, then spin only if needed | Sub-millisecond periods on a loaded system |
| Holding mutex while calling `run()` | Priority inversion, long lock hold times | Release all locks before invoking user callback | `run()` does significant work |
| Creating/joining jthread per period | High overhead, jitter | Keep one worker for periodic timers; only join on stop/destroy | Short periods (< 10 ms) |

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| Static cast of `this` to `void*` passed to thread | Type-safety loss, potential misuse if callback signature changes | Capture `this` directly in the lambda passed to `paraos::jthread` |
| Calling virtual `run()` without join in destructor | Use-after-free at process shutdown | Always stop and join in `~timer()` before returning |
| Unbounded semaphore releases from user-triggered `change_period()` | Denial-of-service via exception | Gate releases with a mutex-protected flag and drain pending signals |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| Timer silently stalls before scheduler starts | User thinks the timer is broken | Document scheduler requirement; assert or return false if scheduler not running |
| `reset()` no longer restarts from "now" | Existing timing logic breaks | Implement deadline re-baselining explicitly |
| `change_period()` takes effect only after current sleep | Control-loop latency increases | Wake worker immediately and recompute deadline |
| Different jitter characteristics on Unix vs Windows/FreeRTOS | Cross-platform tests flake | Accept OS-thread scheduling in standalone test; do not assert microsecond precision |

## "Looks Done But Isn't" Checklist

- [ ] **One-shot mode:** Verify a timer with `is_auto_reload = false` fires exactly once after `start()` and again only after another `start()`.
- [ ] **Periodic mode:** Verify continuous firing at the configured period without accumulated drift over at least 10 iterations.
- [ ] **Stop from `run()`:** Verify `stop()` can be called safely inside the callback without deadlock.
- [ ] **Destructor safety:** Verify a timer with automatic storage can be destroyed while running without crash or sanitizer report.
- [ ] **Period change:** Verify `change_period()` applies the new period to the next interval, not the one after.
- [ ] **Reset semantics:** Verify `reset()` on a running timer re-evaluates expiry relative to the call time.
- [ ] **Start when already running:** Verify `start()` on a running timer re-arms relative to now, matching old POSIX behavior.
- [ ] **Scheduler gate:** Verify the standalone test calls `paraos::jthread::start_scheduler()` and timers fire afterward.
- [ ] **Cross-port isolation:** Verify `port_win/` and `port_freertos/` build without changes.
- [ ] **clang-tidy:** Verify `*_clang_tidy` presets pass with no new warnings from `port_unix/paraos_timer.hpp` or `port_tests/test_timer.cpp`.

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Sleep drift | LOW | Replace `sleep_for(period)` with deadline-based `sleep_until`/`sleep_for(remaining)`. |
| One-shot/periodic mix-up | LOW | Add explicit branch on `is_auto_reload_` after each callback. |
| Deadlock in stop/join | MEDIUM | Detect self-join and use `std::optional<paraos::jthread>` with explicit reset after join. |
| Lost semaphore wakeups | MEDIUM | Introduce a mutex-protected `wakeup_requested` flag and drain the semaphore before each wait. |
| Destructor virtual call race | HIGH | Ensure worker is joined in `~timer()` body; review object lifetime with sanitizers. |
| clang-tidy failure | LOW | Add explicit lambda return types, `[[nodiscard]]`, and narrow `NOLINT` suppressions with rationale. |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Accumulated timing drift | Phase 2 — Core loop | Standalone test measures elapsed time over many periods; stress test repeats 150×. |
| One-shot vs periodic semantics | Phase 2 — Core loop | Test counts callbacks for both modes. |
| Stop/join race and destructor deadlock | Phase 3 — Synchronization | Run TSan/ASan and call `stop()` from inside `run()`. |
| Period change while sleeping | Phase 3 — Synchronization | Test changes period mid-run and checks next interval. |
| `reset()` deadline semantics | Phase 3 — Synchronization | Test resets running timer and measures interval from reset time. |
| Virtual `run()` on destroyed object | Phase 3 — Synchronization | Create/destroy timer in a loop under sanitizers. |
| Scheduler-gating surprise | Phase 4 — Standalone test | `test_timer.cpp` exercises scheduler start explicitly. |
| Semaphore overflow/lost wakeups | Phase 3 — Synchronization | Rapid `change_period()` calls in stress test. |
| Non-monotonic clock usage | Phase 2 — Core loop | Code review for `system_clock`/`CLOCK_REALTIME`; test survives clock adjustment. |
| clang-tidy failures | Phase 5 — Static analysis | `pc_debug_*_clang_tidy` preset passes with no new diagnostics. |

## Sources

- Current implementation: `port_unix/paraos_timer.hpp` (Linux `timer_create`/`timer_settime` and macOS `pthread_cond_timedwait` paths).
- Target primitives: `port_pc/paraos_jthread.hpp`, `port_pc/paraos_mutex_std.hpp`, `port_pc/paraos_semaphore_std.hpp`, `paraos_sleep.hpp`.
- Reference ports: `port_win/paraos_timer.hpp`, `port_freertos/paraos_timer.hpp`.
- Project context: `.planning/PROJECT.md`, milestone v1.10.

---
*Pitfalls research for: Unix timer refactor (v1.10)*
*Researched: 2026-06-25*
