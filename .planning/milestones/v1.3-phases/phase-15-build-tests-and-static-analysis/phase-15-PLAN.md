# Phase 15: build-tests-and-static-analysis - Plan

**Plan ID:** 15-01  
**Phase:** 15  
**Created:** 2026-06-22  
**Status:** Ready for execution

## Goal

Add `port_tests/test_mutex_basic.cpp`, register it in CTest, and verify all relevant presets build and pass tests without new static-analysis warnings.

## Scope

- Create `port_tests/test_mutex_basic.cpp` covering `lock()`, `try_lock()`, `unlock()`, `std::lock_guard`, and `std::unique_lock`.
- Register the test in `port_tests/CMakeLists.txt`.
- Build and run tests for `pc_debug_clang`, `pc_debug_gcc`, `freertos_debug_clang`, `freertos_debug_gcc`.
- Verify `*_clang_tidy` presets do not introduce new warnings from the new test or headers.

## Requirements Addressed

- TEST-01..04: test compiles and passes on PC and FreeRTOS presets.
- TEST-05: tidy presets build without new warnings.

## Tasks

### Task 1: Create test file

**File:** `port_tests/test_mutex_basic.cpp`

- Include `paraos_mutex_std.hpp`, `<mutex>`, `<atomic>`, `<cstdlib>`, `<iostream>`.
- Test `std::lock_guard<paraos::mutex>`.
- Test `std::unique_lock<paraos::mutex>`.
- Test `try_lock()`, `lock()`, `unlock()`.
- Provide FreeRTOS-compatible task entry point and exit via `std::_Exit` on POSIX simulator.

### Task 2: Register test in CMake

**File:** `port_tests/CMakeLists.txt`

- Add `test_mutex_basic` executable modeled after `test_jthread_basic`.
- Set C++20, warnings-as-error, link `paraos::paraos`.
- Add CTest registration.
- Enable clang-tidy when `CLANG_TIDY_ENABLE` is set.

### Task 3: Build and verify

- Build `pc_debug_clang`, `pc_debug_gcc`, `freertos_debug_clang`, `freertos_debug_gcc`.
- Run `ctest` for PC presets.
- Inspect `*_clang_tidy` build output for new warnings attributed to mutex code.

## Risks

- FreeRTOS runtime on macOS POSIX simulator cannot run sequential main logic; the test must execute from a FreeRTOS task and call `std::_Exit`.
- Pre-existing `*_clang_tidy` failures in `containers/` may obscure new warnings; any new warnings from mutex code must be isolated and fixed.
