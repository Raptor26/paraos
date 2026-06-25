# Phase 40: Design and test scaffold - Context

**Gathered:** 2026-06-25
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure phase)

<domain>
## Phase Boundary

Подготовить скелет новой реализации и тестовую инфраструктуру до начала разработки цикла таймера.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — pure infrastructure phase. Use ROADMAP phase goal, success criteria, and codebase conventions to guide decisions.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `port_unix/paraos_timer.hpp` currently contains split Linux (`timer_create`/`timer_settime`/`timer_delete`) and macOS (`pthread_mutex_t`/`pthread_cond_t`/`pthread_create`) implementations.
- `port_win/paraos_timer.hpp` uses Windows `CreateTimerQueueTimer` and is out of scope for changes.
- `port_freertos/paraos_timer.hpp` uses FreeRTOS software timers and is out of scope for changes.
- `paraos::jthread`, `paraos::mutex`, `paraos::binary_semaphore`, and `paraos::sleep_for` are available from previous milestones.

### Established Patterns
- Header-only OSAL with platform folders selected by root `CMakeLists.txt`.
- Standalone tests in `port_tests/` are simple executables linked against `paraos::paraos` with `cxx_std_20`.
- Existing standalone tests: `test_thread_only_*.cpp`, `test_jthread_basic.cpp`, `test_mutex_basic.cpp`, `test_semaphore_std.cpp`.
- PC tests are registered with CTest `TIMEOUT 20`.

### Integration Points
- New `port_tests/test_timer.cpp` must be added to `port_tests/CMakeLists.txt`.
- `port_unix/paraos_timer.hpp` is the only port to be modified; public API must remain unchanged.

</code_context>

<specifics>
## Specific Ideas

No specific requirements — infrastructure phase.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>
