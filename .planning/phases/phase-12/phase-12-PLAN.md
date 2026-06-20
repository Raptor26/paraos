---
phase: 12
name: "Build, tests and static analysis"
wave: 1
depends_on: ["11"]
files_modified:
  - CMakeLists.txt
  - port_tests/CMakeLists.txt
  - port_tests/test_jthread_basic.cpp
autonomous: true
---

# Plan: Phase 12 — Build, tests and static analysis

## Objective

Raise the project-wide C++ standard to C++20, add a basic GoogleTest for `paraos::jthread`, and verify that all PC/FreeRTOS Debug and clang-tidy presets build and pass.

## Must-Haves

- [ ] MH-01: Root `CMakeLists.txt` uses `cxx_std_20` for the `paraos` target.
- [ ] MH-02: `port_tests/test_jthread_basic.cpp` exists and covers basic `jthread` behavior.
- [ ] MH-03: `port_tests/CMakeLists.txt` includes `test_jthread_basic.cpp` in `test_paraos_core`.
- [ ] MH-04: `pc_debug_clang` configures, builds and passes `ctest`.
- [ ] MH-05: `pc_debug_gcc` configures, builds and passes `ctest`.
- [ ] MH-06: `freertos_debug_clang` configures, builds and passes `ctest`.
- [ ] MH-07: `freertos_debug_gcc` configures, builds and passes `ctest`.
- [ ] MH-08: `pc_debug_gcc_clang_tidy` builds without new warnings.
- [ ] MH-09: `freertos_debug_gcc_clang_tidy` builds without new warnings.
- [ ] MH-10: No regressions in existing tests.

## Tasks

### Task 1: Switch project to C++20

**read_first:**
- `CMakeLists.txt`

**acceptance_criteria:**
1. `target_compile_features(paraos PUBLIC cxx_std_17 c_std_11)` changed to `cxx_std_20 c_std_11`.
2. No other breaking CMake changes.

### Task 2: Create `test_jthread_basic.cpp`

**read_first:**
- `port_tests/test_mutex.cpp`
- `port_pc/paraos_jthread.hpp`
- `port_freertos/paraos_jthread.hpp`

**acceptance_criteria:**
1. File created at `port_tests/test_jthread_basic.cpp`.
2. Includes `"paraos_jthread.hpp"` and GTest headers.
3. Tests:
   - `ConstructAndJoin`: thread runs callable with args + stop_token.
   - `RequestStop`: `request_stop()` causes the thread to observe `stop_requested()`.
   - `MoveSemantics`: move constructor transfers joinability.
   - `ThreadAttr`: ThreadAttr constructor compiles and runs (stack/name/priority accepted).
4. Uses `std::atomic` for cross-thread state.
5. Has appropriate `NOLINT` blocks for magic numbers if needed.

### Task 3: Register test in CMake

**read_first:**
- `port_tests/CMakeLists.txt`

**acceptance_criteria:**
1. `test_jthread_basic.cpp` added to `test_paraos_core` source list.
2. `test_paraos_core` clang-tidy properties already apply; no extra changes needed.

### Task 4: Build and test PC presets

**read_first:**
- `CMakePresets.json`

**acceptance_criteria:**
1. `pc_debug_clang` configures (using system Clang) and builds.
2. `ctest --test-dir build/pc_debug_clang/` passes.
3. `pc_debug_gcc` configures (using system GCC/Clang) and builds.
4. `ctest --test-dir build/pc_debug_gcc/` passes.

### Task 5: Build and test FreeRTOS presets

**acceptance_criteria:**
1. `freertos_debug_clang` configures and builds.
2. `ctest --test-dir build/freertos_debug_clang/` passes.
3. `freertos_debug_gcc` configures and builds.
4. `ctest --test-dir build/freertos_debug_gcc/` passes.

### Task 6: Run clang-tidy presets

**acceptance_criteria:**
1. `pc_debug_gcc_clang_tidy` configures and builds without clang-tidy errors.
2. `freertos_debug_gcc_clang_tidy` configures and builds without clang-tidy errors.
3. Any false positives are documented with inline `NOLINT` comments and rationale.

## Verification

- All four Debug presets build and pass `ctest`.
- Both clang-tidy presets build without new warnings.
- `test_jthread_basic.cpp` is registered and runs as part of `test_paraos_core`.

## Risks

- Switching the whole project to C++20 may surface warnings in existing code.
- FreeRTOS tests may time out if the jthread task stack is too small.
- clang-tidy may flag C++20 features or the new forwarding constructors.
- macOS `clang`/`clang++` in PATH may resolve to the Arm cross-compiler; use `/usr/bin/clang++` or `-DCMAKE_CXX_COMPILER` override for PC presets.
