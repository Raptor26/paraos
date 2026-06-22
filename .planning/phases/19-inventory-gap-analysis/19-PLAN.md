# Phase 19: Inventory & gap analysis - Plan

**Plan ID:** 19-01  
**Phase:** 19  
**Created:** 2026-06-22  
**Status:** Ready for execution  
**Mode:** Standard  
**Context:** `.planning/phases/19-inventory-gap-analysis/19-CONTEXT.md`  
**Research:** `.planning/phases/19-inventory-gap-analysis/19-RESEARCH.md`

## Goal

Deliver an audit of multithreaded container tests in `containers/tests/`, a documented mapping of legacy primitives (`paraos::Thread`) to std-like replacements (`paraos::jthread`, `paraos::mutex`, `paraos::*_semaphore`), and a concrete list of `port_freertos` gaps needed for uniform behavior across PC and FreeRTOS.

## Scope

- Audit every multithreaded test file in `containers/tests/`.
- Identify every usage of `paraos::Thread`, `paraos::Mutex`, `MutexGuard`, `paraos::SemaphoreBinary`, and `paraos::SemaphoreCounting`.
- Document the std-like replacement for each legacy primitive and any API-usage differences that affect rewriting.
- Inspect `port_freertos` std-like primitives for behavior gaps that could break migrated tests.
- Produce a single authoritative migration inventory and gap list for Phases 20–22.

This phase does **not** modify source code outside `.planning/`.

## Requirements Addressed

- **ANL-01**: Audit each multithreaded container test for legacy primitives and determine std-like replacement feasibility.
- **ANL-02**: Document API differences between legacy and std-like primitives that affect test rewriting.
- **ANL-03**: Identify the minimum set of `port_freertos` hardening items needed for uniform std-like primitive behavior.

## Out of Scope

- Rewriting test code (Phase 20/21).
- Modifying `port_freertos` implementations (Phase 22).
- Build/test execution as a success criterion (Phase 23).
- Replacing `paraos::CriticalSection` with `paraos::mutex` (deferred per user decision D-09/D-10).

## Research Summary

The std-like primitives required by the v1.5 milestone are already implemented:

- `paraos::jthread` on PC (`port_pc/paraos_jthread.hpp`) and FreeRTOS (`port_freertos/paraos_jthread.hpp`).
- `paraos::mutex` on PC (`port_pc/paraos_mutex_std.hpp`) and FreeRTOS (`port_freertos/paraos_mutex_std.hpp`).
- `paraos::counting_semaphore` / `paraos::binary_semaphore` on PC (`port_pc/paraos_semaphore_std.hpp`) and FreeRTOS (`port_freertos/paraos_semaphore_std.hpp`).

Updated technical research is captured in `19-RESEARCH.md`. Key findings:

- All five multithreaded container tests (`test_queue_blocking_spmc.cpp`, `test_queue_blocking_mpsc.cpp`, `test_queue_blocking_mpmc.cpp`, `test_multi_ringbuff_mpmc.cpp`, `test_message_multithread_many_producer_many_consumers.cpp`) use `paraos::Thread` exclusively; a grep for `paraos::Mutex`, `MutexGuard`, `paraos::SemaphoreBinary`, `paraos::SemaphoreCounting`, `paraos::mutex`, `paraos::binary_semaphore`, and `paraos::counting_semaphore` returned **no matches**. Phase 21 is therefore expected to require no changes in `containers/tests/`.
- The migration removes the watcher thread (`check_test_complete_and_exit`), `RegisterDelegate()`, `Finished()`, `StartScheduler()`, `DeleteAll()`, and `Exit()`. Workers move to a local `std::vector<paraos::jthread>` inside an inner scope; final assertions run after the scope exits; FreeRTOS tests call `std::_Exit(EXIT_SUCCESS)` after the inner scope.
- `paraos::jthread` has no public name getter, so `thread_.GiveName()` debug output must capture the name locally or be simplified.
- `paraos::Thread::DelayMs()` is **not** part of the std-like API. The highest-priority FreeRTOS hardening candidate is a cross-platform delay helper (e.g., `paraos::sleep_for`) or a documented per-test `#ifdef` pattern.
- FreeRTOS `paraos::jthread::join()` relies on `RunTask` signaling `join_sem` before `vTaskDelete(nullptr)`, which is already implemented. The main Phase 22 risk is the interaction between `request_stop()`, blocking queue waits with timeouts, and destructor-issued `join()`.

## Tasks

### Task 1: Read canonical source files

**Scope:** Load every file that the inventory must reference.

**Files to read:**

1. `containers/tests/CMakeLists.txt`
2. `containers/tests/test_queue_blocking_spmc.cpp`
3. `containers/tests/test_queue_blocking_mpsc.cpp`
4. `containers/tests/test_queue_blocking_mpmc.cpp`
5. `containers/tests/test_multi_ringbuff_mpmc.cpp`
6. `containers/tests/test_message_multithread_many_producer_many_consumers.cpp`
7. `port_pc/paraos_jthread.hpp`
8. `port_freertos/paraos_jthread.hpp`
9. `port_pc/paraos_mutex_std.hpp`
10. `port_freertos/paraos_mutex_std.hpp`
11. `port_pc/paraos_semaphore_std.hpp`
12. `port_freertos/paraos_semaphore_std.hpp`
13. `paraos_thread_common.hpp`

**Output:** Working notes on every legacy primitive call site and every std-like API detail relevant to the test patterns.

### Task 2: Audit multithreaded container tests

**Scope:** For each of the five tests, produce a structured entry covering:

- File name and CTest target name(s).
- Whether it is registered with CTest label `stress`.
- Every occurrence of legacy primitives:
  - `paraos::Thread`
  - `thread_delegate_type`
  - `RegisterDelegate()`
  - `Finished()`
  - `StartScheduler()`
  - `DeleteAll()`
  - `Exit()`
  - `paraos::Mutex` / `MutexGuard`
  - `paraos::SemaphoreBinary` / `paraos::SemaphoreCounting`
- Proposed std-like replacement for each occurrence.
- Notes on non-trivial rewrites (e.g., watcher-thread removal, `Finished()` counter logic, final assertion location).

**Output:** Update the inventory table in `19-CONTEXT.md` or append a standalone `INVENTORY.md` if the table grows beyond what is readable in context.

### Task 3: Document API differences between `paraos::Thread` and `paraos::jthread`

**Scope:** Confirm and expand the API gap table from `19-CONTEXT.md` with exact signatures from the headers.

Items to verify:

| Topic | What to confirm |
|-------|-----------------|
| Construction | `paraos::jthread(attr, func, args...)` signature and `ThreadAttr` forwarding |
| Start behavior | `jthread` starts immediately; no `StartScheduler()` equivalent |
| Join | `join()` behavior on PC and FreeRTOS |
| Stop token | `request_stop()` / `stop_requested()` availability and interaction with destructor |
| Delay | Whether `paraos::Thread::DelayMs()` is still usable or should be replaced |
| Name / stack / priority | How `ThreadAttr` maps to `jthread` constructor |
| Destructor | Whether `~jthread()` blocks and under what conditions |

**Output:** Refined API-differences table in the phase artifact.

### Task 4: Inspect `port_freertos` for hardening candidates

**Scope:** Identify the minimum set of FreeRTOS-specific behaviors that must work correctly for the migrated tests.

Areas to inspect:

1. **`paraos::jthread` destructor / `join()`:**
   - Confirm `RunTask` signals `join_sem` before `vTaskDelete(nullptr)`.
   - Confirm `join()` does not deadlock if called after the functor returns.
   - Confirm `join()` handles multiple joiners correctly.

2. **`request_stop()` / `stop_token`:**
   - Confirm `stop_flag` is visible across tasks.
   - Confirm destructor-initiated `request_stop()` does not race with task exit.

3. **`paraos::counting_semaphore`:**
   - Confirm `release(N)` is atomic on FreeRTOS.
   - Confirm `try_acquire_for(std::chrono::duration)` converts to ticks correctly.

4. **Thread name lifetime:**
   - Confirm `ThreadAttr::thread_name` (`std::string_view`) outlives `xTaskCreate`.

5. **`std::chrono` compatibility:**
   - List every `std::chrono` type used by `paraos::jthread`, `paraos::mutex`, and `paraos::*_semaphore` on FreeRTOS.
   - Confirm they compile with the FreeRTOS toolchain.

**Output:** FreeRTOS gap / hardening candidate list with suggested verification for Phase 22.

### Task 5: Draft per-test migration sketch

**Scope:** For each of the five multithreaded tests, write a short pseudo-code sketch showing the new structure after migration.

The sketch must follow the user-decided pattern:

- Remove the watcher thread (`check_test_complete_and_exit`).
- Store workers in `std::vector<paraos::jthread>` inside an inner scope in `main()`.
- Let destructor join all threads.
- Run final assertions after the inner scope exits.
- On FreeRTOS, call `std::_Exit(EXIT_SUCCESS)` after the inner scope; on PC, `return 0`.

**Output:** Migration sketches in the phase artifact.

### Task 6: Verify inventory completeness against requirements

**Scope:** Check that every requirement is satisfied by the artifacts produced.

- **ANL-01:** Inventory covers all five multithreaded tests and every legacy primitive occurrence.
- **ANL-02:** API-differences table is complete enough for an implementer to rewrite tests without re-reading headers.
- **ANL-03:** FreeRTOS gap list identifies concrete hardening items for Phase 22.

**Output:** Update requirement traceability in `REQUIREMENTS.md` if needed; do not mark requirements complete until the phase is verified.

### Task 7: Prepare Phase 20/21/22 handoff notes

**Scope:** Create concise notes for downstream phases.

- For **Phase 20**: ordered list of tests to migrate, per-test gotchas, and the common `main()` skeleton.
- For **Phase 21**: list of synchronization replacements (expected minimal); note any `CriticalSection` decisions.
- For **Phase 22**: ranked list of FreeRTOS hardening candidates with risk and suggested verification.

**Output:** Handoff section in the phase artifact.

## Verification

### Static / Review

- [ ] Every multithreaded test in `containers/tests/` is listed in the inventory.
- [ ] Every legacy primitive occurrence in those tests is identified and mapped to a replacement or marked "no replacement needed".
- [ ] API-differences table includes exact constructor/join/stop-token signatures from the headers.
- [ ] FreeRTOS gap list is actionable and ranked.
- [ ] Migration sketches follow the agreed pattern (inner scope, `std::vector<paraos::jthread>`, `_Exit()` only on FreeRTOS).

### Manual

- [ ] Review the inventory with the user or a peer to confirm no missed `paraos::Thread` usage patterns.
- [ ] Confirm that Phase 21 synchronization work is expected to be minimal based on the inventory.
- [ ] Confirm that the FreeRTOS gap list is feasible within Phase 22 scope.

## Risks and Dependencies

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Hidden `paraos::Mutex` / semaphore usage appears during implementation | Low | Medium | Inventory explicitly searches for these symbols; if any are found, document them and flag Phase 21 |
| FreeRTOS `jthread` destructor behavior differs from assumptions | Medium | High | Inspect `RunTask` / `join_sem` signaling in `port_freertos/paraos_jthread.hpp` directly |
| `std::vector<paraos::jthread>` on FreeRTOS has unexpected lifetime issues | Low | Medium | Include destructor-order verification in Phase 22 gap list |
| User wants to expand scope to `port_tests/` or `extra/tests/` | Low | High | Reference the "Out of Scope" section and the v1.5 milestone boundary |
| macOS POSIX simulator cannot run FreeRTOS tests at runtime | Known | Low | Document compilation-only verification for FreeRTOS; runtime verification remains on Linux/Windows CI |

## Definition of Done

- [ ] All five multithreaded container tests are audited.
- [ ] `19-PLAN.md` and `19-CONTEXT.md` together contain the authoritative inventory and gap list.
- [ ] The inventory is committed to `.planning/phases/19-inventory-gap-analysis/`.
- [ ] `ROADMAP.md` Phase 19 status is updated to reflect that planning is complete and execution can begin.
- [ ] Handoff notes for Phases 20–22 are present and reviewed.

## Notes for Phase 20

- Start with `test_queue_blocking_spmc.cpp` as the simplest pattern; once it compiles and passes, use it as the template for the remaining queue tests.
- `test_multi_ringbuff_mpmc.cpp` has `Finished()` counters and `AssertsForTestComplete()` outside the scheduler — ensure these assertions move to `main()` after the inner scope.
- `test_message_multithread_many_producer_many_consumers.cpp` uses `CriticalSection` around shared `std::vector` updates; keep `CriticalSection` per user decision D-09.

## Notes for Phase 21

- Preliminary inspection found no `paraos::Mutex`, `MutexGuard`, or semaphore usage in the five target tests. If this holds, Phase 21 may be limited to confirming `std::lock_guard<paraos::mutex>` / `std::unique_lock<paraos::mutex>` compile on all ports.

## Notes for Phase 22

- Top candidate: verify `~jthread()` / `join()` signaling order in `port_freertos/paraos_jthread.hpp`.
- Second candidate: decide on and implement a cross-platform delay helper for migrated tests (e.g., `paraos::sleep_for(std::chrono::milliseconds)`) or document the per-test `#ifdef` pattern.
- Third candidate: confirm `request_stop()` visibility and destructor safety.
- Fourth candidate: confirm `counting_semaphore::release(N)` and `try_acquire_for` tick conversion under `vTaskSuspendAll()`.
