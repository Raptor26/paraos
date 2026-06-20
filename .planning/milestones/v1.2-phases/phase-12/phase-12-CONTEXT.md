# Phase 12: Build, tests and static analysis - Context

**Gathered:** 2026-06-20
**Status:** Ready for planning
**Mode:** Auto-generated (autonomous smart discuss)

<domain>
## Phase Boundary

Integrate the new `paraos::jthread` API into the build system and test suite. Raise the minimum C++ standard to C++20, add a basic GoogleTest for `jthread`, register it in CTest, and verify that all relevant CMake presets (PC and FreeRTOS, Debug and clang-tidy) build and pass tests without new warnings.

</domain>

<decisions>
## Implementation Decisions

### CMake Changes
- Update `CMakeLists.txt`: change `target_compile_features(paraos PUBLIC cxx_std_17 c_std_11)` to `cxx_std_20 c_std_11`.
- Add `test_jthread_basic.cpp` to the `test_paraos_core` executable in `port_tests/CMakeLists.txt`.
- Keep other test targets' explicit `cxx_std_17` for now; the library's PUBLIC `cxx_std_20` will propagate to them.

### Test Design
- Test file: `port_tests/test_jthread_basic.cpp`.
- Use GoogleTest (GTest) like the existing `test_paraos_core` tests.
- Cover:
  - Construction and join.
  - `stop_token::stop_requested()` / `stop_possible()`.
  - `request_stop()` followed by join/destruction.
  - Move semantics.
  - `ThreadAttr` constructor (name, stack, priority) where applicable.
- Keep tests deterministic and quick-running; avoid relying on real-time scheduling.

### Platform Considerations
- The test must compile for both PC and FreeRTOS.
- On FreeRTOS, use short delays/yields to allow the created task to run.
- Do not start the FreeRTOS scheduler explicitly; `paraos::jthread` creates tasks that run once the scheduler is started by the test harness (GTest's FreeRTOS integration).

### Static Analysis
- `*_clang_tidy` presets must build without new clang-tidy warnings.
- If false positives appear, document them with inline `NOLINT` comments and rationale.

### Verification
- Build: `pc_debug_clang`, `pc_debug_gcc`, `freertos_debug_clang`, `freertos_debug_gcc`.
- Test: the above presets via `ctest`.
- Static analysis: `pc_debug_gcc_clang_tidy`, `freertos_debug_gcc_clang_tidy`.

### Claude's Discretion
- Exact test names and assertion counts are left to implementation, provided coverage items above are met.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `port_tests/CMakeLists.txt` defines `test_paraos_core` with GTest.
- Existing GTest tests in `port_tests/` show patterns for `TEST(...)` blocks.
- `paraos::jthread` headers are in `port_pc/`, `port_unix/`, `port_win/`, `port_freertos/`.

### Established Patterns
- Tests include the public header directly, e.g., `#include "paraos_jthread.hpp"`.
- Test executables link `paraos::paraos` and `GTest::gtest_main`.
- CTest registration via `add_test(NAME ... COMMAND ...)`.

### Integration Points
- `port_tests/CMakeLists.txt` is the single place to add the new test source.
- Root `CMakeLists.txt` controls the project-wide C++ standard.

</code_context>

<specifics>
## Specific Ideas

- Use `std::atomic` in tests to communicate results from the thread back to the test.
- For `request_stop()` test, have the thread loop until `token.stop_requested()` becomes true, then set an atomic flag.
- For move test, move a `jthread` into another variable and verify the original is no longer joinable.

</specifics>

<deferred>
## Deferred Ideas

- Stress or multithreaded jthread tests — future phase.
- Migration of `extra/` and `containers/` to `jthread` — future milestone.

</deferred>
