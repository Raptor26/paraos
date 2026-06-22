---
wave: 1
depends_on: "20"
files_modified:
  - containers/tests/test_paraos_containers.cpp
  - containers/tests/CMakeLists.txt
autonomous: true
---

# Phase 21: Migrate synchronization primitives in container tests - Plan

**Plan ID:** 21-01
**Phase:** 21
**Created:** 2026-06-22
**Status:** Ready for execution
**Mode:** Standard
**Context:** `.planning/phases/21-migrate-sync-primitives/21-CONTEXT.md`

## Goal

Replace legacy `paraos::Mutex`, `MutexGuard`, `paraos::SemaphoreBinary`, and `paraos::SemaphoreCounting` with std-like `paraos::mutex` and `paraos::*_semaphore` in the multithreaded container tests where semantically appropriate.

## Scope

- Search the five migrated multithreaded container tests for any remaining legacy synchronization primitives.
- If found, replace them with std-like equivalents.
- Add a compile-time smoke test proving `std::lock_guard<paraos::mutex>` and `std::unique_lock<paraos::mutex>` work on all ports.

## Requirements Addressed

- **SYNC-01**: Legacy `paraos::Mutex` / `MutexGuard` replaced with `paraos::mutex` + `std::lock_guard` / `std::unique_lock` where appropriate.
- **SYNC-02**: Legacy `paraos::SemaphoreBinary` / `paraos::SemaphoreCounting` replaced with `paraos::binary_semaphore` / `paraos::counting_semaphore` where appropriate.
- **SYNC-03**: `std::lock_guard<paraos::mutex>` and `std::unique_lock<paraos::mutex>` compile and work on all ports.

## Out of Scope

- Replacing `paraos::CriticalSection` with `paraos::mutex`.
- Changes outside `containers/tests/`.

## Research Summary

The Phase 19 audit confirmed that none of the five migrated multithreaded container tests use `paraos::Mutex`, `MutexGuard`, `paraos::SemaphoreBinary`, or `paraos::SemaphoreCounting`. They use `paraos::CriticalSection` only. Therefore, Phase 21 reduces to:
1. Re-confirming the audit result after Phase 20 changes.
2. Adding a smoke test for `std::lock_guard<paraos::mutex>` / `std::unique_lock<paraos::mutex>` compatibility.

## Tasks

### Task 1: Re-confirm no legacy sync primitives in migrated tests

<read_first>
- containers/tests/test_queue_blocking_spmc.cpp
- containers/tests/test_queue_blocking_mpsc.cpp
- containers/tests/test_queue_blocking_mpmc.cpp
- containers/tests/test_multi_ringbuff_mpmc.cpp
- containers/tests/test_message_multithread_many_producer_many_consumers.cpp
</read_first>

<action>
Run `grep` for `paraos::Mutex`, `MutexGuard`, `paraos::SemaphoreBinary`, and `paraos::SemaphoreCounting` across the five files. Document the result in the phase artifacts.
</action>

<acceptance_criteria>
- Grep returns zero matches for the four legacy sync primitive names in the five target files.
</acceptance_criteria>

### Task 2: Add mutex smoke test to existing GTest executable

<read_first>
- containers/tests/test_paraos_containers.cpp
- containers/tests/CMakeLists.txt
- port_pc/paraos_mutex_std.hpp
- port_freertos/paraos_mutex_std.hpp
</read_first>

<action>
Add a GTest test case to `containers/tests/test_paraos_containers.cpp` (or a new source file linked into the same executable) that:
- Constructs a `paraos::mutex`.
- Locks it with `std::lock_guard<paraos::mutex>`.
- Locks it with `std::unique_lock<paraos::mutex>` and explicitly calls `unlock()`.
- Runs the same operations in a way that compiles on PC and FreeRTOS.
</action>

<acceptance_criteria>
- The smoke test compiles with `pc_debug_clang`, `pc_debug_gcc`, `freertos_debug_clang`, and `freertos_debug_gcc`.
- The smoke test passes when run with `ctest` on PC presets.
- `grep -n "std::lock_guard<paraos::mutex>\|std::unique_lock<paraos::mutex>" containers/tests/*.cpp` returns at least one match.
</acceptance_criteria>

### Task 3: Verify all PC tests pass

<read_first>
- CMakePresets.json
</read_first>

<action>
Build `pc_debug_clang` and `pc_debug_gcc` and run the full container test suite.
</action>

<acceptance_criteria>
- `cmake --build build/pc_debug_clang/` succeeds.
- `ctest --test-dir build/pc_debug_clang/` passes.
- `cmake --build build/pc_debug_gcc/` succeeds.
- `ctest --test-dir build/pc_debug_gcc/` passes.
</acceptance_criteria>

### Task 4: Verify FreeRTOS compilation

<read_first>
- CMakePresets.json
</read_first>

<action>
Build `freertos_debug_clang` and `freertos_debug_gcc` presets. Only compilation is required.
</action>

<acceptance_criteria>
- `cmake --build build/freertos_debug_clang/` succeeds.
- `cmake --build build/freertos_debug_gcc/` succeeds.
</acceptance_criteria>

## Verification

### Static / Review

- [ ] No `paraos::Mutex`, `MutexGuard`, `paraos::SemaphoreBinary`, or `paraos::SemaphoreCounting` remain in the five migrated multithreaded tests.
- [ ] A mutex smoke test exists and uses both `std::lock_guard<paraos::mutex>` and `std::unique_lock<paraos::mutex>`.

### Automated

- [ ] PC presets build and all container tests pass.
- [ ] FreeRTOS presets compile.

## Definition of Done

- [ ] Audit re-confirmed: no legacy sync primitives in target tests.
- [ ] Mutex smoke test added and passing on PC.
- [ ] FreeRTOS presets compile.
- [ ] `21-SUMMARY.md` and `21-VERIFICATION.md` created.

## Artifacts this phase produces

- Modified files:
  - `containers/tests/test_paraos_containers.cpp` (or new smoke test source)
  - Possibly `containers/tests/CMakeLists.txt` if a new source file is added.
- Planning artifacts:
  - `.planning/phases/21-migrate-sync-primitives/21-SUMMARY.md`
  - `.planning/phases/21-migrate-sync-primitives/21-VERIFICATION.md`

## must_haves

### truths
- The five migrated multithreaded container tests contain no `paraos::Mutex`, `MutexGuard`, `paraos::SemaphoreBinary`, or `paraos::SemaphoreCounting`.
- A smoke test demonstrates `std::lock_guard<paraos::mutex>` and `std::unique_lock<paraos::mutex>` compile on PC and FreeRTOS.
- PC presets pass all container tests.
- FreeRTOS presets compile without new errors.

### prohibitions
- statement: Do not replace `paraos::CriticalSection` with `paraos::mutex` in this phase.
  status: resolved
  verification: `grep -c "paraos::CriticalSection" containers/tests/test_*.cpp` returns the same count as before Phase 21.
