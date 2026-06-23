# Phase 34: Migrate `extra/tests` standalone executables and CMake - Context

**Gathered:** 2026-06-23
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure migration)

<domain>
## Phase Boundary

Standalone multithread tests in `extra/tests/` use the modern `paraos::jthread` lifecycle, and `extra/tests/CMakeLists.txt` builds them with C++20 and clang-tidy.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — pure infrastructure/build migration phase.

- Ensure `test_oneshot_executor_thread.cpp`, `test_paraos_thread_sequence.cpp`, and `test_paraos_cooperative_scheduling_thread.cpp` contain no references to the legacy `paraos::Thread` class.
- Ensure standalone tests use `paraos::jthread::start_scheduler()` / `end_scheduler()` and RAII cleanup.
- Update `extra/tests/CMakeLists.txt` to compile standalone targets with `cxx_std_20`.
- Ensure `CXX_CLANG_TIDY` is attached to all standalone targets when `CLANG_TIDY_ENABLE` is on.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- Migrated standalone tests from Phases 31-33 already use `paraos::jthread` lifecycle.
- `extra/tests/CMakeLists.txt` already has clang-tidy wiring for two of the three standalone targets.

### Established Patterns
- `paraos::jthread::start_scheduler()` / `end_scheduler()` replace `paraos::Thread::StartScheduler()` / `DeleteAll()`.
- Local `paraos::jthread` objects provide RAII cleanup.

### Integration Points
- Standalone targets are built only when `THREAD_ENABLE` is on.

</code_context>

<specifics>
## Specific Ideas

No specific requirements — infrastructure migration phase. Refer to ROADMAP phase description and success criteria.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>
