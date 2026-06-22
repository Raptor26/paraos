# Phase 27: Build and static analysis verification - Context

**Gathered:** 2026-06-22
**Status:** Ready for planning

## Phase Boundary

Run the full verification matrix for milestone v1.6:

- PC Debug Clang and GCC presets build and pass `ctest --timeout 20`.
- PC GCC clang-tidy preset builds without new warnings from `paraos::jthread` code.
- FreeRTOS Clang and GCC presets build; their multithread tests at least compile.
- Special attention to `test_jthread_basic` and the updated container multithread tests.

## Implementation Decisions

### PC verification
- **D-01:** Run `cmake --preset pc_debug_clang` / `pc_debug_gcc` from a clean state, then `ctest --test-dir build/<preset> --output-on-failure --stop-on-failure --timeout 20`.
- **D-02:** Capture full ctest output to a log file.

### Static analysis
- **D-03:** Run `pc_debug_gcc_clang_tidy` preset and grep the build log for warnings in `port_pc/paraos_jthread.hpp`, `port_freertos/paraos_jthread.hpp`, and the updated test files.
- **D-04:** Any new clang-tidy warnings introduced by this milestone must be fixed or documented with inline suppression.

### FreeRTOS verification
- **D-05:** Build `freertos_debug_clang` and `freertos_debug_gcc`.
- **D-06:** Runtime execution on macOS POSIX simulator is not required; build-only is acceptable because of the known environment limitation.

## Canonical References

- `.planning/REQUIREMENTS.md` — BLD-01, BLD-02, BLD-03.
- `.planning/ROADMAP.md` — Phase 27 scope and success criteria.

## Existing Code Insights

### Reusable Assets
- CMake presets are already defined in `CMakePresets.json`.
- The modified files are `port_pc/paraos_jthread.hpp`, `port_freertos/paraos_jthread.hpp`, and the six updated test files.

### Established Patterns
- CI uses `ctest --output-on-failure --stop-on-failure --schedule-random --timeout 20`.

## Specific Ideas

- For clang-tidy, focus only on warnings that originate from files touched in this milestone; pre-existing warnings in third-party or unrelated files are out of scope.

## Deferred Ideas

- Windows runtime verification remains deferred.
