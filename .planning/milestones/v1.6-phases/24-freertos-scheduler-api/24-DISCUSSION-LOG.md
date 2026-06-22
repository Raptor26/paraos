# Phase 24: FreeRTOS scheduler API - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-22
**Phase:** 24-FreeRTOS scheduler API
**Areas discussed:** PC port declaration timing, Safety of end_scheduler(), Protection against repeated start_scheduler() calls, Runtime limitation documentation

---

## PC port declaration timing

| Option | Description | Selected |
|--------|-------------|----------|
| Only FreeRTOS now | Do not touch PC header; PC implementation is Phase 25. | ✓ |
| PC stubs now | Add no-op `start_scheduler()`/`end_scheduler()` and `is_scheduler_running()` returning `true` to PC header now. | |
| Common base interface | Introduce a shared `paraos_jthread_base.hpp` that both ports implement. | |

**User's choice:** Only FreeRTOS now.
**Notes:** SCHED-01 is phrased for all ports, but ROADMAP assigns PC work to Phase 25. FreeRTOS presets will validate the new API via compilation only.

### Follow-up: test changes in Phase 24

| Option | Description | Selected |
|--------|-------------|----------|
| Leave as-is | Keep `test_jthread_basic.cpp` using `paraos::Thread::StartScheduler()` and `std::_Exit()`. | |
| Minimal step | Replace FreeRTOS branch calls with new `jthread` methods. | |
| Do not touch tests in Phase 24 | New methods are validated only by compiling FreeRTOS presets. | ✓ |

**User's choice:** Do not touch tests in Phase 24.
**Notes:** Test unification is Phase 26 scope.

---

## Safety of end_scheduler()

| Option | Description | Selected |
|--------|-------------|----------|
| Direct delegation | Forward to `vTaskEndScheduler()` without checks, like `Thread::Exit()`. | |
| Check `IsSchedulerRunning()` | Verify scheduler state before calling `vTaskEndScheduler()`. | ✓ |
| Assert / configASSERT | Fail in Debug if scheduler is not running. | |
| Document limitations only | Keep direct delegation but add Doxygen notes. | |

**User's choice:** Check `IsSchedulerRunning()`.
**Notes:** Avoids undefined behavior when scheduler is not started.

### Follow-up: signature of end_scheduler()

| Option | Description | Selected |
|--------|-------------|----------|
| void + guard | Simple, matches `Thread::Exit()`, no return value. | |
| bool | Return `true` if `vTaskEndScheduler()` was called, `false` if scheduler was not running. | ✓ |
| void + trace message | No return value, but trace when call is ignored. | |

**User's choice:** bool.
**Notes:** Allows callers to observe whether shutdown actually happened.

---

## Protection against repeated start_scheduler() calls

| Option | Description | Selected |
|--------|-------------|----------|
| User responsibility | Direct delegation like `Thread::StartScheduler()`. | |
| Assert / configASSERT | Fail on second call. | ✓ |
| Ignore repeated calls | Silently do nothing if already running. | |
| Return bool | Return `true` on first call, `false` on repeats. | |

**User's choice:** Assert / configASSERT on second call.
**Notes:** Double protection preferred.

### Follow-up: guard implementation

| Option | Description | Selected |
|--------|-------------|----------|
| configASSERT only | Use FreeRTOS assert if available. | |
| PARAOS_CHECK_ASSERT duplicate | Add PARAOS-level check plus configASSERT. | ✓ |
| Custom atomic bool flag | Track initialized state independently. | |

**User's choice:** Duplicate guard via PARAOS_CHECK_ASSERT.
**Notes:** Adds standalone Debug-level protection on top of FreeRTOS configASSERT.

### Follow-up: signature of start_scheduler()

| Option | Description | Selected |
|--------|-------------|----------|
| void | Guard/assert makes return value unnecessary. | ✓ |
| bool | Return whether scheduler was started. | |

**User's choice:** void.
**Notes:** Consistent with legacy `Thread::StartScheduler()`.

---

## Runtime limitation documentation

| Option | Description | Selected |
|--------|-------------|----------|
| STATE.md / PROJECT.md only | Rely on existing environment limitation notes. | |
| Doxygen in header methods | Add `@note`/`@warning` to new methods in `port_freertos/paraos_jthread.hpp`. | ✓ |
| Inline comments near guards | Explain guard/assert rationale. | ✓ |
| All three places | Header Doxygen + inline comments + existing STATE/PROJECT notes. | ✓ |

**User's choice:** All three places.
**Notes:** Initially the user selected option 2 (Doxygen in header + note in test file), but after a scope guard reminder that tests should not be touched in Phase 24, the choice was refined to Doxygen in the header plus inline comments, with existing STATE.md/PROJECT.md notes remaining authoritative.

---

## Claude's Discretion

None — all decisions were explicitly selected by the user.

## Deferred Ideas

None — discussion stayed within Phase 24 scope.
