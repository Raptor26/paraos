# Phase 31: Migrate `OneShotExecutor` to `paraos::jthread` - Context

**Gathered:** 2026-06-23
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure migration)

<domain>
## Phase Boundary

`extra/paraos_oneshot_executor.hpp` uses `paraos::jthread` internally while preserving its public API.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — pure infrastructure migration phase. Follow existing codebase conventions and jthread patterns from `port_pc/paraos_jthread.hpp` and migrated `port_tests/test_thread_only_*.cpp`.

- Replace `paraos::Thread thread_` with `paraos::jthread thread_`.
- Move the delegate-processing loop inside the jthread callable.
- `Finish()` must request stop, unblock the blocking queue, and join gracefully.
- Preserve `thread_start_flag` semantics where feasible; jthread blocks on scheduler gate when scheduler is not running.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `paraos::jthread` in `port_pc/paraos_jthread.hpp` provides stop-token-aware threads and scheduler gating.
- `paraos::Thread` legacy API in `port_unix/paraos_thread.hpp` shows old delegate/scheduler lifecycle.
- `paraos::QueueBlocking` and `paraos::IQueueBlocking` already used by `IOneShotExecutor`.

### Established Patterns
- jthread callables receive `paraos::stop_token` as last argument and loop until `stop_requested()`.
- `paraos::jthread::start_scheduler()` / `paraos::jthread::end_scheduler()` replace `paraos::Thread::StartScheduler()` / `DeleteAll()` in migrated code.
- Standalone multithread tests in `port_tests/` and `containers/tests/` already migrated to jthread.

### Integration Points
- `extra/paraos_oneshot_executor.hpp` is included by `extra/tests/test_oneshot_executor.cpp` (GoogleTest, no scheduler) and `extra/tests/test_oneshot_executor_thread.cpp` (standalone, uses scheduler).

</code_context>

<specifics>
## Specific Ideas

No specific requirements — infrastructure migration phase. Refer to ROADMAP phase description and success criteria.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>
