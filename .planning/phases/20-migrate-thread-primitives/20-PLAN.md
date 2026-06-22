---
wave: 1
depends_on: "19"
files_modified:
  - containers/tests/test_queue_blocking_spmc.cpp
  - containers/tests/test_queue_blocking_mpsc.cpp
  - containers/tests/test_queue_blocking_mpmc.cpp
  - containers/tests/test_multi_ringbuff_mpmc.cpp
  - containers/tests/test_message_multithread_many_producer_many_consumers.cpp
  - containers/tests/CMakeLists.txt
autonomous: true
---

# Phase 20: Migrate thread primitives in container tests - Plan

**Plan ID:** 20-01
**Phase:** 20
**Created:** 2026-06-22
**Status:** Ready for execution
**Mode:** Standard
**Context:** `.planning/phases/20-migrate-thread-primitives/20-CONTEXT.md`

## Goal

Replace `paraos::Thread`, `thread_delegate_type`, `RegisterDelegate`, `Finished`, `StartScheduler`, `DeleteAll`, and `Exit` with `paraos::jthread` in all five multithreaded container tests.

## Scope

- Rewrite `test_queue_blocking_spmc.cpp`, `test_queue_blocking_mpsc.cpp`, `test_queue_blocking_mpmc.cpp`, `test_multi_ringbuff_mpmc.cpp`, and `test_message_multithread_many_producer_many_consumers.cpp`.
- Remove watcher threads and `ExitFromTest` delegates.
- Use local `std::vector<paraos::jthread>` with RAII join.
- Run final assertions after the inner scope.
- Use `std::_Exit(EXIT_SUCCESS)` on FreeRTOS and `return 0` on PC.
- Replace `paraos::Thread::DelayMs()` with local `#ifdef`-based `SleepMs()` helper.
- Add `TIMEOUT 10` to each multithreaded test target in `containers/tests/CMakeLists.txt`.

## Requirements Addressed

- **THR-01**: `test_queue_blocking_spmc.cpp`, `test_queue_blocking_mpsc.cpp`, `test_queue_blocking_mpmc.cpp` use `paraos::jthread`.
- **THR-02**: `test_multi_ringbuff_mpmc.cpp` uses `paraos::jthread`.
- **THR-03**: `test_message_multithread_many_producer_many_consumers.cpp` uses `paraos::jthread`.
- **THR-04**: No calls to `StartScheduler()`, `DeleteAll()`, `Exit()`, or `Finished()`; finalization via RAII join or `std::_Exit()` on FreeRTOS.

## Out of Scope

- Replacing `paraos::CriticalSection` with `paraos::mutex`.
- Introducing a public `paraos::sleep_for` wrapper (Phase 22).
- Modifying container implementations themselves.
- Changes outside `containers/tests/` except CMake timeout.

## Tasks

### Task 1: Add CTest timeouts

<read_first>
- containers/tests/CMakeLists.txt
</read_first>

<action>
Add `TIMEOUT 10` to each of the five `set_tests_properties(... PROPERTIES LABELS stress)` calls for the multithreaded container tests.
</action>

<acceptance_criteria>
- `containers/tests/CMakeLists.txt` contains `TIMEOUT 10` on the same lines as the stress labels for `test_message_multithread_many_producer_many_consumers`, `test_multi_ringbuff_mpmc`, `test_queue_blocking_spmc`, `test_queue_blocking_mpsc`, and `test_queue_blocking_mpmc`.
- `grep -n "TIMEOUT 10" containers/tests/CMakeLists.txt` returns at least 5 matches.
</acceptance_criteria>

### Task 2: Migrate `test_queue_blocking_spmc.cpp`

<read_first>
- containers/tests/test_queue_blocking_spmc.cpp
- port_pc/paraos_jthread.hpp
- port_freertos/paraos_jthread.hpp
- paraos_thread_common.hpp
- port_freertos/paraos_utils.hpp
</read_first>

<action>
- Remove `#include "paraos_thread.hpp"` and replace with `#include "paraos_jthread.hpp"`.
- Remove the `check_test_complete_and_exit` thread and `ExitFromTest()` function.
- Convert `Producer` and `Consumer` to stateful functors with `operator()(const paraos::stop_token&)`; remove `paraos::Thread` member and `RegisterDelegate()`.
- Store the thread name in a local member for debug output.
- Replace `thread_.Finished(); break;` with `return;`.
- Replace `paraos::Thread::DelayMs(ms)` with a local `SleepMs(ms)` helper.
- In `main()`, create workers in a local `std::vector<paraos::jthread>` inner scope; push one producer and five consumers.
- Run `CheckIfTestSuccessfullyComplete()` after the inner scope.
- On FreeRTOS, call `std::_Exit(EXIT_SUCCESS)`; on PC, `return 0`.
</action>

<acceptance_criteria>
- `grep -c "paraos::Thread" containers/tests/test_queue_blocking_spmc.cpp` returns 0.
- `grep -c "paraos::jthread" containers/tests/test_queue_blocking_spmc.cpp` is greater than 0.
- `grep -c "StartScheduler\|DeleteAll\|RegisterDelegate\|Finished()" containers/tests/test_queue_blocking_spmc.cpp` returns 0.
- File compiles with `pc_debug_clang` preset and the test passes `ctest --output-on-failure -R test_queue_blocking_spmc`.
</acceptance_criteria>

### Task 3: Migrate `test_queue_blocking_mpsc.cpp`

<read_first>
- containers/tests/test_queue_blocking_mpsc.cpp
- containers/tests/test_queue_blocking_spmc.cpp (after Task 2)
</read_first>

<action>
Apply the same migration as Task 2 to `test_queue_blocking_mpsc.cpp`. Five producers, one consumer. Compute `total_items_to_be_pushed` before constructing threads.
</action>

<acceptance_criteria>
- `grep -c "paraos::Thread" containers/tests/test_queue_blocking_mpsc.cpp` returns 0.
- `grep -c "StartScheduler\|DeleteAll\|RegisterDelegate\|Finished()" containers/tests/test_queue_blocking_mpsc.cpp` returns 0.
- File compiles with `pc_debug_clang` preset and the test passes `ctest --output-on-failure -R test_queue_blocking_mpsc`.
</acceptance_criteria>

### Task 4: Migrate `test_queue_blocking_mpmc.cpp`

<read_first>
- containers/tests/test_queue_blocking_mpmc.cpp
- containers/tests/test_queue_blocking_spmc.cpp (after Task 2)
</read_first>

<action>
Apply the same migration pattern. Three producers, three consumers. Compute `expected_total_items_in_queue` before constructing threads.
</action>

<acceptance_criteria>
- `grep -c "paraos::Thread" containers/tests/test_queue_blocking_mpmc.cpp` returns 0.
- `grep -c "StartScheduler\|DeleteAll\|RegisterDelegate\|Finished()" containers/tests/test_queue_blocking_mpmc.cpp` returns 0.
- File compiles with `pc_debug_clang` preset and the test passes `ctest --output-on-failure -R test_queue_blocking_mpmc`.
</acceptance_criteria>

### Task 5: Migrate `test_multi_ringbuff_mpmc.cpp`

<read_first>
- containers/tests/test_multi_ringbuff_mpmc.cpp
- containers/tests/test_queue_blocking_spmc.cpp (after Task 2)
</read_first>

<action>
- Apply the same migration pattern.
- Remove the duplicate `AssertsForTestComplete()` call after the old `StartScheduler()`/`DeleteAll()` location.
- Keep the final `AssertsForTestComplete()` call after the inner `jthread` scope in `main()`.
</action>

<acceptance_criteria>
- `grep -c "paraos::Thread" containers/tests/test_multi_ringbuff_mpmc.cpp` returns 0.
- `grep -c "StartScheduler\|DeleteAll\|RegisterDelegate\|Finished()" containers/tests/test_multi_ringbuff_mpmc.cpp` returns 0.
- `grep -c "AssertsForTestComplete" containers/tests/test_multi_ringbuff_mpmc.cpp` returns 1.
- File compiles with `pc_debug_clang` preset and the test passes `ctest --output-on-failure -R test_multi_ringbuff_mpmc`.
</acceptance_criteria>

### Task 6: Migrate `test_message_multithread_many_producer_many_consumers.cpp`

<read_first>
- containers/tests/test_message_multithread_many_producer_many_consumers.cpp
- containers/tests/test_queue_blocking_spmc.cpp (after Task 2)
</read_first>

<action>
- Apply the same migration pattern.
- Preserve all explicit `ThreadAttr.priority` values for consumers and producers.
- Keep `paraos::CriticalSection` around shared `std::vector` updates.
</action>

<acceptance_criteria>
- `grep -c "paraos::Thread" containers/tests/test_message_multithread_many_producer_many_consumers.cpp` returns 0.
- `grep -c "StartScheduler\|DeleteAll\|RegisterDelegate\|Finished()" containers/tests/test_message_multithread_many_producer_many_consumers.cpp` returns 0.
- File compiles with `pc_debug_clang` preset and the test passes `ctest --output-on-failure -R test_message_multithread_many_producer_many_consumers`.
</acceptance_criteria>

### Task 7: Verify all PC presets build and tests pass

<read_first>
- CMakePresets.json
</read_first>

<action>
Configure and build `pc_debug_clang` and `pc_debug_gcc` presets. Run `ctest` for the container multithreaded tests.
</action>

<acceptance_criteria>
- `cmake --build build/pc_debug_clang/` succeeds.
- `ctest --test-dir build/pc_debug_clang/ -L stress --output-on-failure --timeout 20` passes all five tests.
- `cmake --build build/pc_debug_gcc/` succeeds.
- `ctest --test-dir build/pc_debug_gcc/ -L stress --output-on-failure --timeout 20` passes all five tests.
</acceptance_criteria>

### Task 8: Verify FreeRTOS presets compile

<read_first>
- CMakePresets.json
</read_first>

<action>
Configure and build `freertos_debug_clang` and `freertos_debug_gcc` presets. Only compilation is required; runtime is environment-limited.
</action>

<acceptance_criteria>
- `cmake --build build/freertos_debug_clang/` succeeds.
- `cmake --build build/freertos_debug_gcc/` succeeds.
- No new compilation errors appear in the migrated test files.
</acceptance_criteria>

## Verification

### Static / Review

- [ ] No `paraos::Thread`, `thread_delegate_type`, `RegisterDelegate`, `Finished()`, `StartScheduler()`, `DeleteAll()`, or `paraos::Thread::Exit()` remain in the five target files.
- [ ] Each target file uses `paraos::jthread` and `std::vector<paraos::jthread>`.
- [ ] Each target file has an inner scope in `main()` and runs final assertions after it.
- [ ] FreeRTOS-only `std::_Exit(EXIT_SUCCESS)` is present at the end of each `main()`.
- [ ] `TIMEOUT 10` is present for all five multithreaded CTest targets.

### Automated

- [ ] `pc_debug_clang` builds and stress tests pass.
- [ ] `pc_debug_gcc` builds and stress tests pass.
- [ ] `freertos_debug_clang` and `freertos_debug_gcc` compile without new errors.

## Risks and Dependencies

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Worker functor lifetime issues with captured references | Low | High | Capture by reference only objects that outlive the inner scope; final assertions run after join. |
| FreeRTOS `vTaskDelay` not available without `task.h` include | Low | Medium | Include `task.h` in the FreeRTOS branch of the local helper. |
| `std::vector<paraos::jthread>` move behavior differs on FreeRTOS | Low | Medium | Use `emplace_back` directly; avoid reallocation after thread start. |
| Test flakiness from immediate thread start vs. old scheduler start | Medium | Medium | CTest timeout and existing queue timeouts absorb timing differences; observe stress runs. |

## Definition of Done

- [ ] All five multithreaded tests use `paraos::jthread`.
- [ ] No legacy thread API calls remain in those files.
- [ ] `containers/tests/CMakeLists.txt` adds `TIMEOUT 10` to each multithreaded test.
- [ ] PC presets build and stress tests pass.
- [ ] FreeRTOS presets compile.
- [ ] `20-SUMMARY.md` and `20-VERIFICATION.md` are created.

## Artifacts this phase produces

- Modified C++ files:
  - `containers/tests/test_queue_blocking_spmc.cpp`
  - `containers/tests/test_queue_blocking_mpsc.cpp`
  - `containers/tests/test_queue_blocking_mpmc.cpp`
  - `containers/tests/test_multi_ringbuff_mpmc.cpp`
  - `containers/tests/test_message_multithread_many_producer_many_consumers.cpp`
- Modified CMake file:
  - `containers/tests/CMakeLists.txt`
- Planning artifacts:
  - `.planning/phases/20-migrate-thread-primitives/20-SUMMARY.md`
  - `.planning/phases/20-migrate-thread-primitives/20-VERIFICATION.md`

## must_haves

### truths
- All five target files contain `paraos::jthread` and no `paraos::Thread`.
- `paraos::Thread::StartScheduler()`, `DeleteAll()`, `Exit()`, `RegisterDelegate()`, and `Finished()` are absent from the five target files.
- Each migrated test has a `std::vector<paraos::jthread>` inner scope in `main()`.
- Final assertions run after the inner scope exits.
- FreeRTOS tests call `std::_Exit(EXIT_SUCCESS)` after final assertions; PC tests `return 0`.
- `containers/tests/CMakeLists.txt` adds `TIMEOUT 10` to each multithreaded test target.

### prohibitions
- statement: Do not replace `paraos::CriticalSection` with `paraos::mutex` in this phase.
  status: resolved
  verification: `grep -c "paraos::mutex" containers/tests/test_*.cpp` returns 0 before Phase 21.
- statement: Do not introduce a public `paraos::sleep_for` wrapper in this phase.
  status: resolved
  verification: No new file named `paraos_sleep.hpp` or similar is created in this phase.
