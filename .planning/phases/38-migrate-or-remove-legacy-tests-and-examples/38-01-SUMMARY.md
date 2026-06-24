# Phase 38: Migrate or remove legacy tests and examples — Summary

**Completed:** 2026-06-24
**Status:** Complete

## What Changed

- Deleted legacy-only tests that exercised removed `paraos::Mutex` / `paraos::Semaphore*` APIs:
  - `port_tests/test_mutex.cpp`
  - `port_tests/test_mutex_raii.cpp`
  - `port_tests/test_semaphore.cpp`
- Deleted deeply legacy-bound test:
  - `port_tests/test_thread_create_then_delete_many_threads.cpp`
- Deleted legacy-bound standalone examples:
  - `port_tests/example_timer.cpp`
  - `port_tests/example_paraos_timer.cpp`
  - `port_tests/example_thread_check_timeout.cpp`
  - `port_tests/example_socket_udp.cpp`
  - `port_tests/example_paraos_socket_udp.cpp`
- Updated `port_tests/test_jthread_basic.cpp` to remove the obsolete `#include "paraos_thread.hpp"`.
- Updated `port_tests/CMakeLists.txt`:
  - Removed target definitions for deleted tests/examples.
  - Removed deleted source files from `test_paraos_core`.
  - Removed dangling `CXX_CLANG_TIDY` properties for deleted targets.

## Verification

- `cmake --build build/pc_debug_clang/` succeeds.
- `ctest --test-dir build/pc_debug_clang/` passes 58/58 tests.

## Notes

- The deleted tests/examples are superseded by existing std-like tests (`test_mutex_basic`, `test_semaphore_std`, `test_jthread_basic`, etc.).
- `port_tests/test_thread_only_*.cpp` continue to use `paraos::ThreadAttr` for `paraos::jthread` construction and remain intact.
