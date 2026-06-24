# Roadmap: PARAOS

## Milestones

- ✅ **v1.0 macOS Support** — Phases 1-4 (shipped earlier)
- ✅ **v1.1 Static Analysis Cleanup** — Phases 5-8 (shipped 2026-06-20) — see `.planning/milestones/v1.1-ROADMAP.md`
- ✅ **v1.2 std::jthread-style Thread API** — Phases 9-12 (shipped 2026-06-20) — see `.planning/milestones/v1.2-ROADMAP.md`
- ✅ **v1.3 std::mutex-style Mutex API** — Phases 13-15 (shipped 2026-06-22) — see `.planning/milestones/v1.3-ROADMAP.md`
- ✅ **v1.4 std::semaphore-style Semaphore API** — Phases 16-18 (shipped 2026-06-22) — see `.planning/milestones/v1.4-ROADMAP.md`
- ✅ **v1.5 Modernize container tests on std-like primitives** — Phases 19-23 (shipped 2026-06-22) — see `.planning/milestones/v1.5-ROADMAP.md`
- ✅ **v1.6 paraos::jthread scheduler control** — Phases 24-27 (shipped 2026-06-22) — see `.planning/milestones/v1.6-ROADMAP.md`
- ✅ **v1.7 Migrate `test_thread_only_*` to `paraos::jthread`** — Phases 28-30 (shipped 2026-06-23) — see `.planning/milestones/v1.7-ROADMAP.md`
- ✅ **v1.8 Migrate `extra/` libraries to `paraos::jthread`** — Phases 31-35 (shipped 2026-06-23) — see `.planning/milestones/v1.8-ROADMAP.md`
- 🔄 **v1.9 Remove legacy Thread/Mutex/Semaphore implementations** — Phases 36-39

## Phases

<details>
<summary>✅ v1.8 Migrate `extra/` libraries to `paraos::jthread` (Phases 31-35) — SHIPPED 2026-06-23</summary>

- [x] **Phase 31: Migrate `OneShotExecutor` to `paraos::jthread`** — Internal thread wrapper replaced; queue delegate loop runs inside jthread callable.
- [x] **Phase 32: Migrate `ThreadSequence` to `paraos::jthread`** — Internal thread wrapper replaced; timer tick loop runs inside jthread callable.
- [x] **Phase 33: Migrate `CooperativeScheduling` to `paraos::jthread`** — Internal thread wrapper replaced; scheduler loop runs inside jthread callable.
- [x] **Phase 34: Migrate `extra/tests` standalone executables and CMake** — Standalone thread tests use `paraos::jthread`, `start_scheduler()`/`end_scheduler()`; CMakeLists updated to C++20 and clang-tidy.
- [x] **Phase 35: Build, static analysis and runtime verification** — All PC/FreeRTOS presets pass ctest; clang-tidy presets clean; no regressions.

</details>

### v1.9 Remove legacy Thread/Mutex/Semaphore implementations (Phases 36-39)

- [x] **Phase 36: Remove legacy implementation headers** — Delete `port_*/paraos_thread.hpp`, `port_*/paraos_mutex.hpp`, `port_*/paraos_semaphore.hpp`; clean `paraos_thread_common.hpp`; remove `paraos_mutex_raii.hpp`.
- [x] **Phase 37: Migrate internal consumers to std-like primitives** — Update `containers/`, `port_unix/paraos_critical.hpp`, socket UDP headers, and any remaining internal references.
- [x] **Phase 38: Migrate or remove legacy tests and examples** — Rewrite/delete `test_thread_create_then_delete_many_threads.cpp`, `test_mutex.cpp`, `test_mutex_raii.cpp`, `test_semaphore.cpp`, and the five `example_*.cpp` files that use `paraos::Thread`/`paraos::SemaphoreBinary`.
- [ ] **Phase 39: Build, static analysis and regression verification** — All PC/FreeRTOS presets configure and build; `ctest` passes on PC; `*_clang_tidy` presets remain clean.

## Phase Details

### Phase 31: Migrate `OneShotExecutor` to `paraos::jthread`
**Goal:** `extra/paraos_oneshot_executor.hpp` uses `paraos::jthread` internally while preserving its public API.
**Depends on:** Nothing (first phase of milestone)
**Requirements:** MIG-01
**Success Criteria** (what must be TRUE):
  1. `IOneShotExecutor` owns a `paraos::jthread` member instead of `paraos::Thread`.
  2. The delegate-processing loop runs inside the jthread callable.
  3. `Finish()` signals stop and joins the thread gracefully on both PC and FreeRTOS.
  4. Public `EnqueueDelegate` overloads remain unchanged.
**Plans**: TBD

### Phase 32: Migrate `ThreadSequence` to `paraos::jthread`
**Goal:** `extra/paraos_thread_sequence.hpp` uses `paraos::jthread` internally while preserving its public API.
**Depends on:** Phase 31
**Requirements:** MIG-02
**Success Criteria** (what must be TRUE):
  1. `IThreadSequence` owns a `paraos::jthread` member instead of `paraos::Thread`.
  2. The timer tick loop (`Run()`) runs inside the jthread callable and observes `stop_token`.
  3. `Finish()` requests stop and joins without relying on `Finished()`/`Base*` deferred deletion.
  4. `NotifyGive()` continues to wake the sequence thread.
**Plans**: TBD

### Phase 33: Migrate `CooperativeScheduling` to `paraos::jthread`
**Goal:** `extra/paraos_thread_cooperative_scheduling.hpp` uses `paraos::jthread` internally while preserving its public API.
**Depends on:** Phase 32
**Requirements:** MIG-03
**Success Criteria** (what must be TRUE):
  1. `ICooperativeScheduling` owns a `paraos::jthread` member instead of `paraos::Thread`.
  2. The scheduler loop (`Run()`) runs inside the jthread callable and observes `stop_token`.
  3. `Finish()` exits the scheduler, requests stop, and joins the thread.
  4. `AddTask()` and `SetIdleCallback()` keep existing signatures.
**Plans**: TBD

### Phase 34: Migrate `extra/tests` standalone executables and CMake
**Goal:** Standalone multithread tests in `extra/tests/` use the modern `paraos::jthread` lifecycle.
**Depends on:** Phase 33
**Requirements:** MIG-04, BUILD-01, BUILD-02
**Success Criteria** (what must be TRUE):
  1. `test_oneshot_executor_thread.cpp`, `test_paraos_thread_sequence.cpp`, and `test_paraos_cooperative_scheduling_thread.cpp` no longer reference `paraos::Thread`.
  2. Tests use `paraos::jthread::start_scheduler()` / `end_scheduler()` and RAII cleanup.
  3. `extra/tests/CMakeLists.txt` compiles standalone targets with `cxx_std_20`.
  4. `CXX_CLANG_TIDY` is attached to standalone targets when `CLANG_TIDY_ENABLE` is on.
**Plans**: TBD

### Phase 35: Build, static analysis and runtime verification
**Goal:** All PC and FreeRTOS presets remain green after the migration.
**Depends on:** Phase 34
**Requirements:** BUILD-03, TEST-01, TEST-02
**Success Criteria** (what must be TRUE):
  1. `pc_debug_clang`, `pc_debug_gcc`, and `pc_debug_gcc_clang_tidy` presets configure, build, and pass `ctest`.
  2. `freertos_debug_clang` and `freertos_debug_gcc` presets compile.
  3. No new clang-tidy warnings appear in modified `extra/` headers or tests.
  4. `ctest` count on PC matches or exceeds the pre-milestone baseline (58 tests).
**Plans**: TBD

### Phase 36: Remove legacy implementation headers
**Goal:** Delete legacy `paraos::Thread`, `paraos::Mutex`, and `paraos::Semaphore*` implementation headers while preserving common attributes needed by `paraos::jthread`.
**Depends on:** Nothing (first phase of v1.9)
**Requirements:** REM-01, REM-02, REM-03, REM-04, REM-05
**Success Criteria** (what must be TRUE):
  1. `port_unix/paraos_thread.hpp`, `port_unix/paraos_mutex.hpp`, `port_unix/paraos_semaphore.hpp` are removed.
  2. `port_win/paraos_thread.hpp`, `port_win/paraos_mutex.hpp`, `port_win/paraos_semaphore.hpp` are removed.
  3. `port_freertos/paraos_thread.hpp`, `port_freertos/paraos_mutex.hpp`, `port_freertos/paraos_semaphore.hpp` are removed.
  4. `paraos_thread_common.hpp` still provides `ThreadAttr`, `ThreadPriority`, and other shared definitions used by `paraos::jthread`.
  5. `paraos_mutex_raii.hpp` is removed or rewritten in terms of `std::lock_guard<paraos::mutex>` / `std::unique_lock<paraos::mutex>`.
**Plans**: TBD

### Phase 37: Migrate internal consumers to std-like primitives
**Goal:** Update remaining internal code that depends on legacy `paraos::Thread`, `paraos::Mutex`, or `paraos::Semaphore*` to use the std-like replacements.
**Depends on:** Phase 36
**Requirements:** MIG-01, MIG-02, MIG-03, MIG-04
**Success Criteria** (what must be TRUE):
  1. `containers/paraos_message_buffer.hpp` and `containers/paraos_queue_blocking.hpp` use `paraos::mutex` and `paraos::*_semaphore`.
  2. `port_unix/paraos_critical.hpp` uses `paraos::mutex` and no longer references `MutexBase`.
  3. `port_unix/paraos_thread.hpp` and `port_win/paraos_thread.hpp` either are forwarding headers to `paraos_jthread.hpp` or are removed entirely.
  4. `port_unix/paraos_socket_udp.hpp` and `port_win/paraos_socket_udp.hpp` use `paraos::jthread` / `paraos::sleep_for` instead of `paraos::Thread`.
**Plans**: TBD

### Phase 38: Migrate or remove legacy tests and examples
**Goal:** Eliminate remaining test/example code that uses legacy `paraos::Thread`, `paraos::Mutex`, or `paraos::Semaphore*`.
**Depends on:** Phase 37
**Requirements:** TEST-01, TEST-02, TEST-03
**Success Criteria** (what must be TRUE):
  1. `port_tests/test_thread_create_then_delete_many_threads.cpp`, `test_mutex.cpp`, `test_mutex_raii.cpp`, and `test_semaphore.cpp` are deleted or rewritten to use std-like primitives.
  2. `port_tests/example_thread_check_timeout.cpp`, `example_timer.cpp`, `example_paraos_timer.cpp`, `example_socket_udp.cpp`, and `example_paraos_socket_udp.cpp` no longer reference `paraos::Thread` or `paraos::SemaphoreBinary`.
  3. `port_tests/CMakeLists.txt` is updated to remove deleted targets and keep/register the remaining std-like tests/examples.
**Plans**: TBD

### Phase 39: Build, static analysis and regression verification
**Goal:** All PC and FreeRTOS presets remain green after legacy removal.
**Depends on:** Phase 38
**Requirements:** BUILD-01, BUILD-02, BUILD-03, BUILD-04
**Success Criteria** (what must be TRUE):
  1. `pc_debug_clang`, `pc_debug_gcc`, and `pc_debug_gcc_clang_tidy` presets configure, build, and pass `ctest`.
  2. `freertos_debug_clang` and `freertos_debug_gcc` presets compile.
  3. `*_clang_tidy` presets produce no new warnings from changed or deleted code.
  4. PC `ctest` count is at least the pre-milestone baseline (58 tests) unless a test is intentionally removed with documented rationale.
**Plans**: TBD

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
| ----- | --------- | -------------- | ------ | --------- |
| 1-4. macOS Support | v1.0 | 4/4 | Complete | earlier |
| 5. Reproduce & Classify clang-tidy warnings | v1.1 | 1/1 | Complete | 2026-06-20 |
| 6. Fix Core, Headers & port_unix | v1.1 | 1/1 | Complete | 2026-06-20 |
| 7. Fix Tests, Examples & Document Suppressions | v1.1 | 1/1 | Complete | 2026-06-20 |
| 8. Regression Guard | v1.1 | 1/1 | Complete | 2026-06-20 |
| 9. PC jthread implementation | v1.2 | 1/1 | Complete | 2026-06-20 |
| 10. FreeRTOS jthread implementation | v1.2 | 1/1 | Complete | 2026-06-20 |
| 11. Thread attributes integration | v1.2 | 1/1 | Complete | 2026-06-20 |
| 12. Build, tests and static analysis | v1.2 | 1/1 | Complete | 2026-06-20 |
| 13. PC mutex implementation | v1.3 | 1/1 | Complete | 2026-06-22 |
| 14. FreeRTOS mutex implementation | v1.3 | 1/1 | Complete | 2026-06-22 |
| 15. Build, tests and static analysis | v1.3 | 1/1 | Complete | 2026-06-22 |
| 16. PC counting_semaphore implementation | v1.4 | 1/1 | Complete | 2026-06-22 |
| 17. FreeRTOS counting_semaphore implementation | v1.4 | 1/1 | Complete | 2026-06-22 |
| 18. Build, tests and static analysis | v1.4 | 1/1 | Complete | 2026-06-22 |
| 19. Inventory & gap analysis | v1.5 | 1/1 | Complete | 2026-06-20 |
| 20. Migrate thread primitives in container tests | v1.5 | 1/1 | Complete | 2026-06-22 |
| 21. Migrate synchronization primitives in container tests | v1.5 | 1/1 | Complete | 2026-06-22 |
| 22. FreeRTOS std-like primitives hardening | v1.5 | 1/1 | Complete | 2026-06-22 |
| 23. Build, tests and static analysis | v1.5 | 1/1 | Complete | 2026-06-22 |
| 24. FreeRTOS scheduler API | v1.6 | 2/2 | Complete | 2026-06-22 |
| 25. PC scheduler state and gating | v1.6 | 3/3 | Complete | 2026-06-22 |
| 26. Test unification | v1.6 | 7/7 | Complete | 2026-06-22 |
| 27. Build and static analysis verification | v1.6 | 6/6 | Complete | 2026-06-22 |
| 28. Migrate `test_thread_only_*` sources to `paraos::jthread` | v1.7 | 1/1 | Complete | 2026-06-23 |
| 29. Update `port_tests/CMakeLists.txt` for new tests | v1.7 | 1/1 | Complete | 2026-06-23 |
| 30. Runtime verification on macOS | v1.7 | 1/1 | Complete | 2026-06-23 |
| 31. Migrate `OneShotExecutor` to `paraos::jthread` | v1.8 | 1/1 | Complete | 2026-06-23 |
| 32. Migrate `ThreadSequence` to `paraos::jthread` | v1.8 | 1/1 | Complete | 2026-06-23 |
| 33. Migrate `CooperativeScheduling` to `paraos::jthread` | v1.8 | 1/1 | Complete | 2026-06-23 |
| 34. Migrate `extra/tests` standalone executables and CMake | v1.8 | 1/1 | Complete | 2026-06-23 |
| 35. Build, static analysis and runtime verification | v1.8 | 1/1 | Complete | 2026-06-23 |
| 36. Remove legacy implementation headers | v1.9 | 1/1 | Complete | 2026-06-24 |
| 37. Migrate internal consumers to std-like primitives | v1.9 | 1/1 | Complete | 2026-06-24 |
| 38. Migrate or remove legacy tests and examples | v1.9 | 1/1 | Complete | 2026-06-24 |
| 39. Build, static analysis and regression verification | v1.9 | 0/1 | Not started | — |
