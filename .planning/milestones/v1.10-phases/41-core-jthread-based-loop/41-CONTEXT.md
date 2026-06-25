# Phase 41: Core jthread-based loop - Context

**Gathered:** 2026-06-25
**Status:** Ready for planning
**Mode:** Auto-generated (requirements-driven phase)

<domain>
## Phase Boundary

Реализовать рабочий поток таймера поверх PARAOS-примитивов, поддерживающий периодический и one-shot режимы без дрейфа.

</domain>

<decisions>
## Implementation Decisions

### Clock and timing
- Use `std::chrono::steady_clock` for deadlines to avoid wall-clock jumps.
- Compute the next deadline as `now + period_ms` at the start of each period.
- For periodic mode, after `run()` returns, recompute the deadline as the previous deadline + period (catch-up logic) to avoid drift, capping catch-up to one period to prevent bursts after stalls.

### Worker thread lifecycle
- `std::optional<paraos::jthread> worker_` is created lazily inside `start()` and destroyed inside `stop()` by assigning `std::nullopt`.
- The worker loop exits when `is_stop_requested_` becomes true.

### Waiting strategy
- Use `paraos::binary_semaphore::try_acquire_for(remaining_time)` to sleep until the next deadline or until woken by `change_period()`, `reset()`, or `stop()`.
- Wake the worker by calling `wake_sem_.release()` after mutating state under `mutex_`.

### Mutex usage
- Hold `mutex_` only to read/write mutable state (`period_ms_`, `is_running_`, `is_stop_requested_`, deadline storage).
- Release `mutex_` before calling the user-provided virtual `run()`.
- Do not hold `mutex_` across `try_acquire_for()` — the semaphore provides synchronization for wakeups.

### Modes
- `is_auto_reload_ == true`: after `run()` returns, schedule the next invocation; if the deadline has already passed, run immediately and advance the deadline by period.
- `is_auto_reload_ == false`: after `run()` returns once, set `is_running_ = false` and exit the loop.

### Error handling
- If `run()` throws, catch the exception inside the worker loop, stop the timer, and do not rethrow (mirrors the existing non-throwing public API contract).

### Claude's Discretion
- Exact variable naming for the deadline storage.
- Whether to log timer stop/exception events (no logging in this phase to keep the implementation minimal).
- Minor details of catch-up capping behavior (e.g., one period vs. immediate next).

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- Phase 40 scaffold already introduced `paraos::mutex mutex_`, `paraos::binary_semaphore wake_sem_{0}`, `std::optional<paraos::jthread> worker_`, `bool is_running_{false}`, `bool is_stop_requested_{false}` in `port_unix/paraos_timer.hpp`.
- `paraos::jthread` provides `request_stop()` and join-on-destruction semantics via `std::jthread` on PC/Unix.
- `paraos::binary_semaphore` provides `try_acquire_for()` with `std::chrono` durations.

### Established Patterns
- `extra/paraos_oneshot_executor.hpp` uses `std::optional<paraos::jthread>` for deferred thread creation.
- `port_unix/paraos_timer.hpp` (Phase 40) removed all POSIX/pthread code and now has placeholder methods returning `isr_bool{true}`.

### Integration Points
- `port_unix/paraos_timer.hpp` is the only file to modify.
- Public API must remain identical to Phase 40.
- Standalone test `port_tests/test_timer.cpp` will be expanded in Phase 43; Phase 41 only needs the timer to compile and run internally.

</code_context>

<specifics>
## Specific Ideas

- Implement the loop in a private `timer_loop()` method invoked by the `paraos::jthread` worker.
- Store the deadline as `std::chrono::steady_clock::time_point next_deadline_` (add as private member).
- In `start()`:
  - If already running, update `next_deadline_ = now + period_ms_` and release semaphore.
  - If not running, set `is_running_ = true`, `is_stop_requested_ = false`, create `worker_` with `timer_loop`.
- In `stop()`:
  - Set `is_stop_requested_ = true`, release semaphore, assign `worker_ = std::nullopt` to join.
- In `change_period()`:
  - Update `period_ms_` under lock; if running, recompute `next_deadline_ = now + period_ms_` and release semaphore.

</specifics>

<deferred>
## Deferred Ideas

- Synchronization edge cases (safe stop from inside `run()`, destructor safety) are deferred to Phase 42.
- Expanded test coverage (periodic accuracy, one-shot, change_period, reset) is deferred to Phase 43.
- Cross-platform regression validation is deferred to Phase 44.

</deferred>
