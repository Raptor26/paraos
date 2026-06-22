# Phase 26: Test unification - Context

**Gathered:** 2026-06-22
**Status:** Ready for planning

## Phase Boundary

Rewrite multithread tests so they use a single cross-platform pattern based on
`paraos::jthread::start_scheduler()` / `paraos::jthread::end_scheduler()`.

- Remove `std::_Exit()` and `#ifdef PARAOS_LIKE_FREERTOS` platform branches.
- Replace platform-specific delays with `paraos::sleep_for(std::chrono::milliseconds{...})`.
- `test_jthread_basic.cpp` must pass on PC.
- Container multithread tests must compile and run on PC.

## Implementation Decisions

### Cross-platform test pattern
- **D-01:** Each test creates its worker `paraos::jthread` instances, then a
  "stopper" `paraos::jthread` that waits for the test completion condition and
  calls `paraos::jthread::end_scheduler()`.
- **D-02:** Main calls `paraos::jthread::start_scheduler()` and then waits on a
  condition variable that the stopper signals after `end_scheduler()` returns.
- **D-03:** Main does **not** call `join()` on individual threads; the stopper
  (via `end_scheduler()`) joins them, and the destructors skip already-joined
  threads.

### PC end_scheduler self-join
- **D-04:** `port_pc/paraos_jthread.hpp::end_scheduler()` skips the calling
  thread when joining, so the stopper thread can safely shut down the scheduler.

### FreeRTOS behavior
- **D-05:** FreeRTOS `end_scheduler()` keeps its current implementation (calls
  `vTaskEndScheduler()`). The stopper thread invokes it; `vTaskStartScheduler()`
  returns; main continues.
- **D-06:** Runtime execution of FreeRTOS tests remains host-limited; build-only
  verification is acceptable on macOS POSIX simulator.

### Stop conditions
- **D-07:** Each test uses its existing global completion counter as the stopper
  condition (e.g. `pop_item_cnt >= expected_total_items_in_queue`,
  `consumer_thread_exit_cnt == consumer_thread_numb`).

## Canonical References

- `.planning/REQUIREMENTS.md` — TEST-01, TEST-02, TEST-03, TEST-04.
- `.planning/ROADMAP.md` — Phase 26 scope.
- `port_tests/test_jthread_basic.cpp` — reference unified test.
- `port_pc/paraos_jthread.hpp` — scheduler API to call from tests.
- `port_freertos/paraos_jthread.hpp` — scheduler API reference.

## Existing Code Insights

### Reusable Assets
- `paraos::sleep_for` is already used in container tests.
- Existing completion counters (`pop_item_cnt`, `consumer_thread_exit_cnt`, etc.)
  provide deterministic stop conditions.

### Established Patterns
- Tests create threads in an inner scope so destructors run before final
  assertions.
- Debug printing uses `PrintDebug` macro and `paraos::OsProfiler`.

### Integration Points
- `paraos::jthread::start_scheduler()` replaces `paraos::Thread::StartScheduler()`.
- `paraos::jthread::end_scheduler()` replaces `std::_Exit()` for FreeRTOS
  shutdown.

## Specific Ideas

- Keep `PrintDebug` and profiler instrumentation unchanged to minimize churn.
- Add small helper functions (`NotifySchedulerEnded`, `WaitForSchedulerEnded`)
  in each test's anonymous namespace.

## Deferred Ideas

- Windows runtime verification remains deferred due to lack of Windows host.
