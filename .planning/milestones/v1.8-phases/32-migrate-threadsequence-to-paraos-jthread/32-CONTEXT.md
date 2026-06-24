# Phase 32: Migrate `ThreadSequence` to `paraos::jthread` - Context

**Gathered:** 2026-06-23
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure migration)

<domain>
## Phase Boundary

`extra/paraos_thread_sequence.hpp` uses `paraos::jthread` internally while preserving its public API.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — pure infrastructure migration phase. Follow patterns established in Phase 31 (`extra/paraos_oneshot_executor.hpp`):

- Replace `paraos::Thread thread_` with `paraos::jthread thread_`.
- Move the periodic tick loop (`Run()`) inside the jthread callable and observe `paraos::stop_token`.
- `Finish()` must request stop, unblock the sequence semaphore (`NotifyGive(false)`), and join gracefully.
- Remove deferred-delete (`is_dynamic`) logic that relied on legacy `paraos::Base*` / `Thread::Finished()`.
- Preserve `NotifyGive(bool is_isr)`, `Register`, `Unregister`, `SetFreq`, `GetMainFreq`, `GiveRegisteredDelegatesNumb` signatures.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `paraos::jthread` in `port_pc/paraos_jthread.hpp`.
- Phase 31 migration of `IOneShotExecutor` to jthread (`extra/paraos_oneshot_executor.hpp`).
- `etl::callback_timer` already used by `IThreadSequence`.

### Established Patterns
- jthread callables receive `const paraos::stop_token&` and loop until `stop_requested()`.
- `NotifyGive()` wakes the sequence thread via `SemaphoreBinary::Give()`.
- Standalone tests use `paraos::jthread::start_scheduler()` / `end_scheduler()` and `std::atomic` for completion signaling.

### Integration Points
- `extra/paraos_thread_sequence.hpp` is used by `extra/tests/test_paraos_thread_sequence.cpp` and `RegisteredDelegates<>` helper.

</code_context>

<specifics>
## Specific Ideas

No specific requirements — infrastructure migration phase. Refer to ROADMAP phase description and success criteria.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>
