# Phase 24: FreeRTOS scheduler API - Context

**Gathered:** 2026-06-22
**Status:** Ready for planning

## Phase Boundary

This phase adds static scheduler-control methods to `paraos::jthread` for the FreeRTOS port only: `start_scheduler()`, `end_scheduler()`, and `is_scheduler_running()`. The methods delegate directly to FreeRTOS scheduler API (`vTaskStartScheduler`, `vTaskEndScheduler`, `xTaskGetSchedulerState`). PC implementation, test unification, and PC scheduler gating are explicitly out of scope for this phase.

## Implementation Decisions

### PC port declaration timing
- **D-01:** Only the FreeRTOS port receives the new scheduler API in Phase 24. PC stubs or implementation are deferred to Phase 25.
- **D-02:** `port_tests/test_jthread_basic.cpp` is not modified in Phase 24. New methods are validated only by compiling the `freertos_debug_clang` and `freertos_debug_gcc` presets.

### Safety of `end_scheduler()`
- **D-03:** `end_scheduler()` checks whether the scheduler is running (`xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED`) before calling `vTaskEndScheduler()`.
- **D-04:** `end_scheduler()` returns `bool`: `true` if `vTaskEndScheduler()` was called, `false` if the scheduler was not running.

### Protection against repeated `start_scheduler()` calls
- **D-05:** `start_scheduler()` guards against repeated calls with `PARAOS_CHECK_ASSERT(xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)`. A final `configASSERT` is also acceptable as a last line of defense.
- **D-06:** `start_scheduler()` returns `void`; the guard/assert makes a return value unnecessary.

### Runtime limitation documentation
- **D-07:** Limitations of the FreeRTOS POSIX port on macOS are documented in the Doxygen comments of the new methods inside `port_freertos/paraos_jthread.hpp` (`@note`/`@warning`):
  - `start_scheduler()` does not return while the scheduler is running.
  - `end_scheduler()` behavior depends on the target FreeRTOS port's implementation of `vPortEndScheduler`.
  - Runtime validation of FreeRTOS tasks is not available on the macOS POSIX simulator.
- **D-08:** Existing runtime limitation notes in `.planning/STATE.md` and `.planning/PROJECT.md` remain authoritative; no separate note is added to `test_jthread_basic.cpp`.

### Claude's Discretion
- None — all decisions were explicitly selected.

## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Requirements
- `.planning/REQUIREMENTS.md` — Scheduler API requirements SCHED-01, SCHED-02, SCHED-03, SCHED-07.
- `.planning/ROADMAP.md` — Phase 24 scope, success criteria, and plan list.

### Existing code to study
- `port_freertos/paraos_jthread.hpp` — FreeRTOS `jthread` implementation to extend.
- `port_freertos/paraos_thread.hpp` — Reference for `StartScheduler()`, `Exit()`, and `IsSchedulerRunning()` patterns.
- `port_pc/paraos_jthread.hpp` — PC `jthread` wrapper; do not modify in this phase.
- `port_tests/test_jthread_basic.cpp` — Existing jthread test; do not modify in this phase.

### FreeRTOS API reference
- `port_freertos/FreeRTOS-Kernel/include/task.h` — Declarations of `vTaskStartScheduler`, `vTaskEndScheduler`, and `xTaskGetSchedulerState`.
- `port_freertos/config/unix/FreeRTOSConfig.h` and `port_freertos/config/win/FreeRTOSConfig.h` — `INCLUDE_xTaskGetSchedulerState` is already enabled.

## Existing Code Insights

### Reusable Assets
- `port_freertos/paraos_thread.hpp` already contains `IsSchedulerRunning()` using `xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED`; the same expression should be reused in `paraos::jthread`.
- `paraos_check.h` provides `PARAOS_CHECK_ASSERT` for Debug builds; use it for the repeated-call guard.

### Established Patterns
- FreeRTOS `jthread` uses a heap-allocated `Context` and `Invoker` to support capturing lambdas and `stop_token`; new static scheduler methods do not affect instance state.
- `paraos::Thread::StartScheduler()` and `Exit()` are `static void` and delegate directly to FreeRTOS; `jthread` methods intentionally add safety checks not present in the legacy API.

### Integration Points
- New methods are added as `public static` members of `paraos::jthread` in `port_freertos/paraos_jthread.hpp`.
- The methods must remain header-only and not introduce dependencies on `paraos::Thread` (SCHED-07).
- Verification is build-only for FreeRTOS presets on macOS; runtime execution is not expected due to the POSIX simulator limitation.

## Specific Ideas

- FreeRTOS `vTaskEndScheduler()` requires a valid `vPortEndScheduler()` implementation; document that the caller must ensure the target port supports scheduler shutdown.
- The new Doxygen comments should reference `paraos::Thread::StartScheduler()` behavior only to contrast, not to create a dependency.

## Deferred Ideas

None — discussion stayed within phase scope.

---

*Phase: 24-FreeRTOS scheduler API*
*Context gathered: 2026-06-22*
