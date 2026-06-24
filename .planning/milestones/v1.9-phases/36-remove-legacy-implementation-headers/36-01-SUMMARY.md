# Phase 36: Remove legacy implementation headers — Summary

**Completed:** 2026-06-24
**Status:** Complete

## What Changed

- Removed legacy implementation headers from all three ports:
  - `port_unix/paraos_thread.hpp`
  - `port_unix/paraos_mutex.hpp`
  - `port_unix/paraos_semaphore.hpp`
  - `port_win/paraos_thread.hpp`
  - `port_win/paraos_mutex.hpp`
  - `port_win/paraos_semaphore.hpp`
  - `port_freertos/paraos_thread.hpp`
  - `port_freertos/paraos_mutex.hpp`
  - `port_freertos/paraos_semaphore.hpp`
- Cleaned `paraos_thread_common.hpp`:
  - Removed obsolete `dtor_callback` and `run_` fields from `ThreadAttr`.
  - Removed `#include "paraos_base.hpp"` dependency.
  - Updated comment to reflect `paraos::jthread` usage.
- Rewrote `paraos_mutex_raii.hpp`:
  - Replaced `MutexGuard` implementation over `MutexBase` with `using MutexGuard = std::lock_guard<paraos::mutex>;`.
  - Now includes `paraos_mutex_std.hpp` instead of the legacy `paraos_mutex.hpp`.

## Verification

- `paraos_thread_common.hpp` compiles standalone with `PARAOS_LIKE_UNIX` and C++20.
- All ten legacy headers are confirmed deleted.
- No CMakeLists.txt references to deleted headers remain.

## Notes

- Remaining consumers (containers, `port_unix/paraos_critical.hpp`, socket UDP headers, and legacy tests/examples) will be migrated in Phases 37 and 38.
