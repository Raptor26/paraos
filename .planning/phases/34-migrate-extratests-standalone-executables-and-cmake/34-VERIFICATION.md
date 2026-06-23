# Phase 34: Migrate `extra/tests` standalone executables and CMake - Verification

**Status:** passed
**Date:** 2026-06-23
**Preset:** pc_debug_clang

## Automated Verification

- [x] `extra/tests/test_oneshot_executor_thread.cpp`, `extra/tests/test_paraos_thread_sequence.cpp`, and `extra/tests/test_paraos_cooperative_scheduling_thread.cpp` contain no references to the legacy `paraos::Thread` class.
- [x] Standalone tests use `paraos::jthread::start_scheduler()` / `end_scheduler()` and local `paraos::jthread` objects for RAII cleanup.
- [x] `extra/tests/CMakeLists.txt` compiles the GoogleTest target and all standalone targets with `cxx_std_20`.
- [x] `CXX_CLANG_TIDY` is attached to `test_paraos_thread_sequence`, `test_paraos_cooperative_scheduling_thread`, and `test_paraos_oneshot_executor` when `CLANG_TIDY_ENABLE` is on.
- [x] All three standalone tests and `[PARAOS EXTRA]` GoogleTest suite pass.

## Test Results

```
Test project /Users/raptor/_vcs/paraos/build/pc_debug_clang
    Start 50: [PARAOS EXTRA]:Cooperative.Create
    ...
    Start 58: test_paraos_oneshot_executor
9/9 Test #58: test_paraos_oneshot_executor ..................................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 9
```

## Notes

- Only the legacy `paraos::Thread` class references were removed; `paraos::ThreadPriority`, `paraos::ThreadAttr`, `paraos::ThreadSequence`, and `paraos::ThreadSequenceAttr` remain as expected.

## Release Criteria

- No new compiler warnings or clang-tidy diagnostics in modified files.
- Full preset validation covered in Phase 35.
