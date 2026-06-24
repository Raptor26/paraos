# Phase 39: Build, static analysis and regression verification — Summary

**Completed:** 2026-06-24
**Status:** Complete

## What Changed

- Fixed `modernize-use-scoped-lock` clang-tidy findings introduced by the migration:
  - Replaced `std::lock_guard<paraos::mutex>` with `std::scoped_lock<paraos::mutex>` in `containers/paraos_queue_blocking.hpp`.
  - Updated `paraos_mutex_raii.hpp` to alias `MutexGuard` to `std::scoped_lock<paraos::mutex>`.
  - Updated `port_tests/test_mutex_basic.cpp` to use `std::scoped_lock<paraos::mutex>`.

## Verification

| Preset | Configure | Build | ctest | Notes |
|--------|-----------|-------|-------|-------|
| `pc_debug_clang` | ✓ | ✓ | 58/58 | — |
| `pc_debug_gcc` | ✓ | ✓ | 58/58 | — |
| `pc_debug_gcc_clang_tidy` | ✓ | ✓ | 58/58 | clang-tidy clean |
| `freertos_debug_clang` | ✓ | ✓ | N/A | compile-only on host |
| `freertos_debug_gcc` | ✓ | ✓ | N/A | compile-only on host |

## Notes

- PC `ctest` count matches the pre-milestone baseline of 58 tests.
- No remaining legacy `paraos::Thread`, `paraos::Mutex`, `paraos::SemaphoreBinary`, `paraos::SemaphoreCounting`, `MutexBase`, or `SemaphoreBase` references in project code (excluding `paraos_mutex_raii.hpp` which now wraps std-like locking).
