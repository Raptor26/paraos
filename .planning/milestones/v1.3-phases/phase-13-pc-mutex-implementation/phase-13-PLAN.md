# Phase 13: pc-mutex-implementation - Plan

**Plan ID:** 13-01  
**Phase:** 13  
**Created:** 2026-06-22  
**Status:** Ready for execution

## Goal

Implement `paraos::mutex` for PC platforms (Windows + Unix) as a thin wrapper over `std::mutex` with a `std::mutex`-compatible API.

## Scope

- Create `port_pc/paraos_mutex_std.hpp` with `paraos::mutex`.
- Create forwarding headers `port_unix/paraos_mutex_std.hpp` and `port_win/paraos_mutex_std.hpp`.
- Leave legacy `paraos::Mutex` / `MutexRecursive` untouched.
- Do not add tests in this phase (Phase 15 owns tests).

## Requirements Addressed

- MUTEX-01..03: `lock()`, `try_lock()`, `unlock()`.
- MUTEX-04: non-copyable, non-movable.
- MUTEX-05..06: `std::lock_guard` / `std::unique_lock` compatibility.
- MUTEX-07..09: file layout and forwarding headers.
- MUTEX-11: no platform `#ifdef` in user code.
- BUILD-01: header available from all PC ports.

## Tasks

### Task 1: Create shared PC implementation

**File:** `port_pc/paraos_mutex_std.hpp`

- Define include guard `PARAOS_MUTEX_STD_HPP`.
- Include `<mutex>`.
- Define `namespace paraos { class mutex; }`.
- Provide:
  - `void lock()`
  - `bool try_lock()`
  - `void unlock()`
- Delete copy/move constructors and assignments.
- Default destructor.
- Use private `std::mutex mutex_` member and forward calls.
- Add Doxygen comments matching project style.

### Task 2: Create Unix forwarding header

**File:** `port_unix/paraos_mutex_std.hpp`

- Include guard `PARAOS_UNIX_MUTEX_STD_HPP`.
- `#include "../port_pc/paraos_mutex_std.hpp"`.

### Task 3: Create Windows forwarding header

**File:** `port_win/paraos_mutex_std.hpp`

- Include guard `PARAOS_WIN_MUTEX_STD_HPP`.
- `#include "../port_pc/paraos_mutex_std.hpp"`.

## Verification

- Configure and build `pc_debug_clang` and `pc_debug_gcc` presets.
- Confirm a minimal compile-only snippet using `std::lock_guard<paraos::mutex>` and `std::unique_lock<paraos::mutex>` compiles.
- Confirm `*_clang_tidy` presets produce no new warnings from the new headers.

## Risks

- Header name `paraos_mutex_std.hpp` differs from the generic `paraos_mutex.hpp` used in early requirements; this avoids include-guard collision with legacy `PARAOS_MUTEX_HPP`. REQUIREMENTS.md and ROADMAP.md will be updated to match the actual file name.
