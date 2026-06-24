# Phase 38: Migrate or remove legacy tests and examples - Context

**Gathered:** 2026-06-24
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped) — test/example cleanup

<domain>
## Phase Boundary

Eliminate remaining test/example code that uses legacy `paraos::Thread`, `paraos::Mutex`, or `paraos::Semaphore*`. Either rewrite the tests/examples to use std-like primitives or delete them if their coverage is duplicated by existing std-like tests.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — test/example cleanup phase. Use ROADMAP phase goal, success criteria, and codebase conventions to guide decisions.

### Test disposition
- `port_tests/test_thread_create_then_delete_many_threads.cpp` — rewrite with `paraos::jthread` or remove if redundant.
- `port_tests/test_mutex.cpp` — delete; `paraos::mutex` is covered by standard library tests and existing `test_jthread_basic.cpp`.
- `port_tests/test_mutex_raii.cpp` — delete; `std::lock_guard<paraos::mutex>` covers the same behavior.
- `port_tests/test_semaphore.cpp` — delete; `paraos::counting_semaphore` / `paraos::binary_semaphore` are covered by standard semantics and existing tests.

### Example disposition
- `port_tests/example_thread_check_timeout.cpp`, `example_timer.cpp`, `example_paraos_timer.cpp`, `example_socket_udp.cpp`, `example_paraos_socket_udp.cpp` — update to use `paraos::jthread` and `paraos::*_semaphore` instead of `paraos::Thread` / `paraos::SemaphoreBinary`.

### CMake updates
- Remove deleted targets from `port_tests/CMakeLists.txt`.
- Keep/register remaining std-like tests/examples.

</decisions>

<canonical_refs>
## Canonical References

### Project-level requirements
- `.planning/REQUIREMENTS.md` — Milestone v1.9 requirements TEST-01 through TEST-03.
- `.planning/ROADMAP.md` — Phase 38 success criteria.

### Code conventions
- `AGENTS.md` — Commit style, coding standards, and build/test procedures.
- `.clang-tidy` — Static analysis rules that must remain clean after changes.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `paraos::jthread` and `paraos::*_semaphore` already implemented across all ports.
- `test_jthread_basic.cpp` provides patterns for jthread-based tests.
- `example_timer.cpp` / `example_paraos_timer.cpp` already demonstrate timer usage; they only need their thread/semaphore references updated.

### Established Patterns
- `port_tests/CMakeLists.txt` registers tests with `gtest_discover_tests` for GoogleTest targets and plain executables for standalone examples.
- Standalone examples use manual `main()` with polling loops.

### Integration Points
- `port_tests/CMakeLists.txt` must be updated when targets are removed.
- Examples are not registered as CTest tests but must still compile.

</code_context>

<specifics>
## Specific Ideas

No specific requirements — cleanup phase. Prefer deletion when legacy tests only exercised the removed API and equivalent coverage exists for std-like primitives.

</specifics>

<deferred>
## Deferred Ideas

None — cleanup phase.

</deferred>

---

*Phase: 38-migrate-or-remove-legacy-tests-and-examples*
*Context gathered: 2026-06-24*
