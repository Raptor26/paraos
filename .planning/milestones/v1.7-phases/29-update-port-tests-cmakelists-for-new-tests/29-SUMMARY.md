---
phase: 29
status: complete
requirements-completed:
  - BUILD-01
  - BUILD-02
  - BUILD-03
  - BUILD-04
---

# Phase 29 Summary: Update `port_tests/CMakeLists.txt` for new tests

## What Was Done

- Verified that all four `test_thread_only_*` targets use `cxx_std_20`.
- Verified that all four targets are registered with `CXX_CLANG_TIDY` under `if(CLANG_TIDY_ENABLE)`.
- Added `TIMEOUT 20` to each of the four `add_test(...)` registrations.

## Verification

- `pc_debug_clang`: `ctest -R test_thread_only_ --timeout 20` passes 4/4.
- `pc_debug_gcc_clang_tidy`: clean build with clang-tidy enabled.
- `freertos_debug_clang` and `freertos_debug_gcc`: clean builds.

## Files Changed

- `port_tests/CMakeLists.txt`
