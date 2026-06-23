# Phase 33: Migrate `CooperativeScheduling` to `paraos::jthread` - Context

**Gathered:** 2026-06-23
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure migration)

<domain>
## Phase Boundary

`extra/paraos_thread_cooperative_scheduling.hpp` uses `paraos::jthread` internally while preserving its public API.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — pure infrastructure migration phase. Follow patterns established in Phases 31 and 32:

- Replace `paraos::Thread thread_` with `paraos::jthread thread_`.
- The scheduler loop (`Run()`) runs inside the jthread callable and observes `paraos::stop_token`.
- `Finish()` must request stop, exit the scheduler (`scheduler_.exit_scheduler()`), unblock `Idle()` (`NotifyGive()`), and join gracefully.
- Remove deferred-delete (`is_dynamic` / `Base*`) logic that relied on legacy `Thread::Finished()`.
- Preserve `AddTask(etl::task&)`, `SetIdleCallback(etl::ifunction<void>&)`, `NotifyGive(bool)`, and `GetScheduler()` signatures.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `paraos::jthread` in `port_pc/paraos_jthread.hpp`.
- Phase 31/32 migrations of `IOneShotExecutor` and `IThreadSequence` to jthread.
- `etl::scheduler` / `etl::task` already used by `ICooperativeScheduling`.

### Established Patterns
- jthread callables receive `const paraos::stop_token&` and observe `stop_requested()`.
- `scheduler_.start()` blocks until `scheduler_.exit_scheduler()` is called.
- `Idle()` waits on `new_cycle_ready_sem_`; `NotifyGive()` signals it.
- Standalone tests use `paraos::jthread::start_scheduler()` / `end_scheduler()` and `std::atomic` for completion signaling.

### Integration Points
- `extra/paraos_thread_cooperative_scheduling.hpp` is used by `extra/tests/test_paraos_cooperative_scheduling_thread.cpp` and the `extra/tests/test_paraos_extra.cpp` GoogleTest suite.

</code_context>

<specifics>
## Specific Ideas

No specific requirements — infrastructure migration phase. Refer to ROADMAP phase description and success criteria.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>
