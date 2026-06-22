---
wave: 1
depends_on: "21"
files_modified:
  - paraos_sleep.hpp
  - containers/tests/test_queue_blocking_spmc.cpp
  - containers/tests/test_queue_blocking_mpsc.cpp
  - containers/tests/test_queue_blocking_mpmc.cpp
  - containers/tests/test_multi_ringbuff_mpmc.cpp
  - containers/tests/test_message_multithread_many_producer_many_consumers.cpp
  - CMakeLists.txt
autonomous: true
---

# Phase 22: FreeRTOS std-like primitives hardening - Plan

**Plan ID:** 22-01
**Phase:** 22
**Created:** 2026-06-22
**Status:** Ready for execution
**Mode:** Standard
**Context:** `.planning/phases/22-freertos-hardening/22-CONTEXT.md`

## Goal

Add the minimum FreeRTOS-specific hardening needed for the migrated container tests to behave uniformly across PC and FreeRTOS. The primary deliverable is a cross-platform `paraos::sleep_for` helper that replaces the duplicated local `#ifdef` helpers in the migrated tests.

## Scope

- Create `paraos_sleep.hpp` with `paraos::sleep_for(std::chrono::milliseconds)`.
- Update the five migrated multithreaded container tests to use `paraos::sleep_for` instead of local `SleepMs()` helpers.
- Ensure the helper compiles on PC (Clang/GCC), FreeRTOS (Clang/GCC), and Windows.
- Verify FreeRTOS `paraos::jthread::join()` / `request_stop()` signaling remains correct for the migrated test pattern.

## Requirements Addressed

- **FR-01**: A cross-platform helper function is added for uniform std-like primitive behavior.
- **FR-02**: `paraos::jthread::join()` / `request_stop()` behavior on FreeRTOS is confirmed safe for the test pattern (no code change required unless inspection reveals a defect).
- **FR-03**: `paraos::counting_semaphore::release(N)` / `try_acquire_for` correctness is already covered by Phase 21 smoke tests; this phase ensures no new `std::chrono` issues are introduced.

## Out of Scope

- Rewriting container tests beyond replacing the sleep helper.
- Modifying `paraos::jthread` implementation unless inspection reveals a real defect.
- Adding multiple-joiner support.

## Research Summary

- FreeRTOS `paraos::jthread` already signals `join_sem` before `vTaskDelete(nullptr)`, which satisfies the destructor-issued join scenario used by the migrated tests.
- The main gap is the lack of a cross-platform sleep helper, forcing each migrated test to duplicate `#ifdef` logic.

## Tasks

### Task 1: Create `paraos_sleep.hpp`

<read_first>
- paraos_utils.hpp
- port_freertos/paraos_utils.hpp
- port_pc/paraos_jthread.hpp
- port_freertos/paraos_jthread.hpp
</read_first>

<action>
Create a new public header `paraos_sleep.hpp` in the project root with:
- `namespace paraos { inline void sleep_for(std::chrono::milliseconds duration); }`
- FreeRTOS implementation using `vTaskDelay(PARAOS_ConvertMsToTicks(...))`.
- PC/Unix/Windows implementation using `std::this_thread::sleep_for(...)`.
- Include guards `PARAOS_SLEEP_HPP`.
</action>

<acceptance_criteria>
- `paraos_sleep.hpp` exists in the project root.
- It contains `paraos::sleep_for(std::chrono::milliseconds)`.
- It compiles on `pc_debug_clang`, `pc_debug_gcc`, `freertos_debug_clang`, and `freertos_debug_gcc`.
</acceptance_criteria>

### Task 2: Wire `paraos_sleep.hpp` into the build

<read_first>
- CMakeLists.txt
- setup.cmake
</read_first>

<action>
Ensure `paraos_sleep.hpp` is reachable via the existing include paths. If it is placed in the project root, no CMake changes are needed beyond making sure the root is in the include path. Verify by building a test that includes it.
</action>

<acceptance_criteria>
- A translation unit including `paraos_sleep.hpp` compiles in all four presets.
</acceptance_criteria>

### Task 3: Replace local `SleepMs()` helpers in migrated tests

<read_first>
- containers/tests/test_queue_blocking_spmc.cpp
- containers/tests/test_queue_blocking_mpsc.cpp
- containers/tests/test_queue_blocking_mpmc.cpp
- containers/tests/test_multi_ringbuff_mpmc.cpp
- containers/tests/test_message_multithread_many_producer_many_consumers.cpp
</read_first>

<action>
In each of the five files:
- Remove the local `SleepMs()` helper and the conditional `#include <thread>` / `#include "task.h"`.
- Add `#include "paraos_sleep.hpp"`.
- Replace all `SleepMs(ms)` calls with `paraos::sleep_for(std::chrono::milliseconds(ms))`.
</action>

<acceptance_criteria>
- `grep -RIn "SleepMs" containers/tests/test_queue_blocking_*.cpp containers/tests/test_multi_ringbuff_mpmc.cpp containers/tests/test_message_multithread_many_producer_many_consumers.cpp` returns no matches.
- `grep -RIn "paraos::sleep_for" containers/tests/test_queue_blocking_*.cpp containers/tests/test_multi_ringbuff_mpmc.cpp containers/tests/test_message_multithread_many_producer_many_consumers.cpp` returns at least one match per file.
</acceptance_criteria>

### Task 4: Verify PC builds and tests pass

<read_first>
- CMakePresets.json
</read_first>

<action>
Build `pc_debug_clang` and `pc_debug_gcc` and run the five migrated stress tests plus the container GTest suite.
</action>

<acceptance_criteria>
- `cmake --build build/pc_debug_clang/` succeeds.
- `ctest --test-dir build/pc_debug_clang/ -R "test_message_multithread_many_producer_many_consumers|test_multi_ringbuff_mpmc|test_queue_blocking_spmc|test_queue_blocking_mpsc|test_queue_blocking_mpmc" --output-on-failure --timeout 30` passes.
- `cmake --build build/pc_debug_gcc/` succeeds.
- `ctest --test-dir build/pc_debug_gcc/ -R "test_message_multithread_many_producer_many_consumers|test_multi_ringbuff_mpmc|test_queue_blocking_spmc|test_queue_blocking_mpsc|test_queue_blocking_mpmc" --output-on-failure --timeout 30` passes.
</acceptance_criteria>

### Task 5: Verify FreeRTOS compilation

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

### Task 6: Inspect FreeRTOS jthread join signaling

<read_first>
- port_freertos/paraos_jthread.hpp
</read_first>

<action>
Verify that `RunTask` gives `join_sem` before `vTaskDelete(nullptr)` and that destructor-issued `join()` cannot deadlock when the task has already completed. Document the finding in the phase summary.
</action>

<acceptance_criteria>
- Phase summary contains a statement confirming the join signaling order and that no code change was required.
</acceptance_criteria>

## Verification

### Static / Review

- [ ] `paraos_sleep.hpp` exists and has correct include guards.
- [ ] No local `SleepMs()` helpers remain in the five migrated tests.
- [ ] FreeRTOS jthread join signaling order is documented.

### Automated

- [ ] PC Clang and GCC builds pass and the five migrated tests pass.
- [ ] FreeRTOS Clang and GCC presets compile.

## Definition of Done

- [ ] `paraos_sleep.hpp` added and compiles on all ports.
- [ ] Migrated tests use `paraos::sleep_for` instead of local `SleepMs`.
- [ ] PC presets pass the migrated tests.
- [ ] FreeRTOS presets compile.
- [ ] `22-SUMMARY.md` and `22-VERIFICATION.md` are created.

## Artifacts this phase produces

- New file:
  - `paraos_sleep.hpp`
- Modified files:
  - `containers/tests/test_queue_blocking_spmc.cpp`
  - `containers/tests/test_queue_blocking_mpsc.cpp`
  - `containers/tests/test_queue_blocking_mpmc.cpp`
  - `containers/tests/test_multi_ringbuff_mpmc.cpp`
  - `containers/tests/test_message_multithread_many_producer_many_consumers.cpp`
- Planning artifacts:
  - `.planning/phases/22-freertos-hardening/22-SUMMARY.md`
  - `.planning/phases/22-freertos-hardening/22-VERIFICATION.md`

## must_haves

### truths
- `paraos::sleep_for(std::chrono::milliseconds)` is available in a public header and works on PC and FreeRTOS.
- The five migrated tests no longer contain local `SleepMs()` helpers.
- FreeRTOS `paraos::jthread::join()` returns after `RunTask` signals `join_sem` and self-deletes.
- PC and FreeRTOS presets build without new errors.

### prohibitions
- statement: Do not introduce C++20-only `std::chrono::duration` templates in `paraos_sleep.hpp`.
  status: resolved
  verification: `paraos_sleep.hpp` contains only `std::chrono::milliseconds` overload.
