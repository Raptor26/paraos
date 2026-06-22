# Phase 25: PC scheduler state and gating - Context

**Gathered:** 2026-06-22
**Status:** Ready for planning

## Phase Boundary

This phase reworks the PC implementation of `paraos::jthread` so that it
emulates FreeRTOS scheduler semantics:

- Threads created before `start_scheduler()` block until it is called.
- `start_scheduler()` releases all waiting threads and sets `is_scheduler_running()` to `true`.
- `end_scheduler()` stops and joins all active `paraos::jthread` instances and sets `is_scheduler_running()` to `false`.
- `is_scheduler_running()` reports the scheduler state.

FreeRTOS port and tests are out of scope for this phase.

## Implementation Decisions

### Gating mechanism
- **D-01:** Each `jthread` instance owns a private gate (`std::mutex` + `std::condition_variable` + `bool` flags). This allows a thread to be released individually in the destructor and collectively by `start_scheduler()` / `end_scheduler()`.
- **D-02:** The gate has two flags: `gate_open` (releases the wait) and `should_run` (whether the user callable should execute). `end_scheduler()` can therefore release waiting threads with `should_run=false` so they exit without executing user code.
- **D-03:** Threads created while the scheduler is already running do not wait (`gate_open=true, should_run=true`).
- **D-04:** Threads created after `end_scheduler()` do not wait and do not execute user code (`gate_open=true, should_run=false`).

### Scheduler state
- **D-05:** A static `std::atomic<SchedulerState>` tracks `kNotStarted`, `kRunning`, and `kStopped`. `start_scheduler()` is idempotent once running and no-op after `end_scheduler()`.
- **D-06:** `is_scheduler_running()` returns `s_scheduler_state == kRunning`.
- **D-07:** `end_scheduler()` returns `bool`: `true` if it transitioned from `kRunning` to `kStopped`, `false` otherwise.

### Active-thread registry
- **D-08:** A static registry of `Context*` pointers is maintained so `end_scheduler()` can stop and join every active thread.
- **D-09:** Registry is updated in constructor, destructor, move constructor, and move assignment. Copy is deleted.
- **D-10:** `Context` is a member of `jthread`; its address is stable within the owning object. Move operations update the registry because the `Context` address moves to the destination object.

### Destructor behavior
- **D-11:** The destructor releases the instance's private gate with `should_run=false` before joining, so a thread waiting for the scheduler does not deadlock when its `jthread` object is destroyed.

### Thread-safety and edge cases
- **D-12:** `start_scheduler()` and `end_scheduler()` take a snapshot of the registry under a mutex, then operate on the snapshot. This avoids holding the registry lock while joining threads.
- **D-13:** `end_scheduler()` must not be called from a `jthread` worker thread; that would self-deadlock. This is documented in Doxygen comments.

### API additions
- **D-14:** Add a default constructor to `paraos::jthread` so moved-from and empty states are well-formed, matching `std::jthread`.
- **D-15:** The public static methods are added to the PC `jthread` class with signatures matching the FreeRTOS port.

## Canonical References

- `.planning/REQUIREMENTS.md` — SCHED-04, SCHED-05, SCHED-06.
- `.planning/ROADMAP.md` — Phase 25 scope and success criteria.
- `port_pc/paraos_jthread.hpp` — File to modify.
- `port_freertos/paraos_jthread.hpp` — Reference signatures for `start_scheduler()`, `end_scheduler()`, `is_scheduler_running()`.

## Existing Code Insights

### Reusable Assets
- `port_pc/paraos_jthread.hpp` already wraps `std::jthread`; the new gating layer fits inside the existing `MakeThread()` lambda and `Context` member.
- `paraos::ThreadAttr` and `ApplyAttr()` are reused unchanged.

### Established Patterns
- PC `jthread` stores a `std::jthread thread_` member and a `ThreadAttr attr_`. The refactor replaces these with a `Context` struct that bundles the thread, attributes, and gate state.
- FreeRTOS `jthread` already uses a `Context` struct; PC `Context` will be similar in purpose but use `std::jthread` and `std::condition_variable`.

### Integration Points
- The new methods are public static members of `paraos::jthread` in `port_pc/paraos_jthread.hpp`.
- The change must not break the public API except for the intentional behavioral change: threads now wait for `start_scheduler()`.

## Specific Ideas

- Keep the `Context` struct simple: one mutex/CV for the gate, one atomic/flag for the scheduler state, and the registry as a static `std::unordered_set<Context*>` guarded by a static mutex.
- Verification in this phase is build-only; runtime behavior is validated by the unified tests in Phase 26.

## Deferred Ideas

- Runtime test unification for PC and FreeRTOS is deferred to Phase 26.
- PC runtime verification on Windows is deferred due to lack of Windows host.
