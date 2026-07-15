# Phase 42: Start/stop/reset/change_period synchronization - Context

**Gathered:** 2026-06-25
**Status:** Ready for planning
**Mode:** Auto-generated (requirements-driven phase)

<domain>
## Phase Boundary

Обеспечить корректную синхронизацию публичных методов с рабочим потоком, безопасный stop изнутри `run()` и безопасный деструктор.

</domain>

<decisions>
## Implementation Decisions

### Synchronization model
- Keep the existing lock pattern: `mutex_` guards `period_ms_`, `is_running_`, `is_stop_requested_`, and `next_deadline_`.
- `run()` is always called without holding `mutex_`.
- `stop()` acquires `mutex_` to set flags, then releases it before joining the worker via `worker_ = std::nullopt`.

### Self-deadlock prevention
- `stop()` called from inside `run()` must not deadlock. Because `run()` does not hold `mutex_`, `stop()` can acquire it safely.
- Joining the worker from within the worker is handled by `paraos::jthread`: its destructor detects `std::this_thread::get_id() == owner_id` and skips join.
- After `stop()` returns to `run()`, the worker loop checks `is_stop_requested_` and exits cleanly.

### Destructor safety
- The destructor keeps the existing `stop()` call.
- If the timer is still running, `stop()` joins the worker before the object is destroyed.
- If `stop()` is called from inside `run()` (and therefore from inside the destructor indirectly), the self-deadlock guard above applies.

### Lost / duplicate wakeups
- Use the existing `wake_worker()` helper that drains the binary semaphore before releasing, preventing `semaphore release overflow`.
- Do not add additional state flags for wakeup accounting; rely on re-checking `is_stop_requested_` and deadlines after waking.

### Start on already-running timer
- Keep the Phase 41 behavior: reset `next_deadline_` to `now() + period_ms_` and wake the worker.

### Change period semantics
- Keep the Phase 41 behavior: update `period_ms_`, recompute `next_deadline_` from `now()`, and wake the worker.

### Reset semantics
- Keep the Phase 41 behavior: forward `reset()` to `start()`.

### Claude's Discretion
- Exact placement of `is_running_ = false` assignments in edge paths.
- Whether to add a small defensive delay or yield in `stop()` when called from inside `run()` (avoid to keep implementation minimal).
- Minor naming or helper extraction for clarity.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- Phase 41 implemented `timer_loop()`, lazy `start()`, `stop()`, `change_period()`, `reset()`, and `wake_worker()` in `port_unix/paraos_timer.hpp`.
- `paraos::jthread` destructor skips join when called from the worker thread itself.

### Established Patterns
- Lock is acquired only for short mutable-state sections.
- `std::optional<paraos::jthread>` controls worker lifetime.
- `paraos::binary_semaphore` is used for wakeup signaling.

### Integration Points
- Only `port_unix/paraos_timer.hpp` is modified.
- Standalone test `port_tests/test_timer.cpp` will be expanded in Phase 43; Phase 42 may add a minimal safety check for stop-from-run if it helps verify the hardening.

</code_context>

<specifics>
## Specific Ideas

- Verify that `stop()` from inside `run()` does not deadlock by inspection and a minimal standalone test.
- Verify destructor safety by instantiating a started timer in a local scope and letting it go out of scope.
- Confirm that repeated `start()` / `stop()` / `change_period()` cycles do not leak threads or overflow the semaphore.
- If any race is found during verification, fix it inside `port_unix/paraos_timer.hpp` with minimal changes.

</specifics>

<deferred>
## Deferred Ideas

- Full accuracy and mode coverage tests are deferred to Phase 43.
- Cross-platform regression validation is deferred to Phase 44.

</deferred>
