---
gsd_state_version: 1.0
milestone: v1.9
milestone_name: "Remove legacy Thread/Mutex/Semaphore implementations"
current_phase: 39
status: verifying
stopped_at: Phase 39 complete; milestone ready for audit
last_updated: "2026-06-24T13:30:00.000Z"
last_activity: 2026-06-24
last_activity_desc: Phase 39 complete — all presets verified
progress:
  total_phases: 4
  completed_phases: 4
  total_plans: 4
  completed_plans: 4
  percent: 100
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-06-24)

**Core value:** Кроссплатформенная переносимость PARAOS сохраняется: код, работающий на Linux/Windows/FreeRTOS, продолжает работать, а новая macOS-разработка ведётся на равных с остальными платформами, включая статический анализ clang-tidy.
**Current focus:** Milestone v1.9 — remove legacy Thread/Mutex/Semaphore implementations.

## Current Position

Phase: 39 — Complete
Plan: 39-01
Status: Verifying / Ready for milestone audit
Last activity: 2026-06-24 — Phase 39 complete; all presets verified

## Accumulated Context

### Decisions

- Migrate `extra/` helpers to `paraos::jthread` while preserving public APIs.
- Keep changes inside `port_unix` with macro isolation (`__APPLE__` / `__linux__`).
- Use macOS-native substitutes for missing POSIX timers.
- Map public `ThreadPriority` enum values 1..7 to the macOS `SCHED_RR` range internally.
- Replace deprecated unnamed POSIX semaphores on macOS with a `pthread_cond_t` + counter backend.
- Validate every available CMake preset on this machine.
- Preserve `.clang-tidy` check set; only add documented false-positive suppressions.
- `jthread` API поверх `std::jthread` для PC и собственная реализация для FreeRTOS.
- Capturing lambdas для FreeRTOS через heap-allocated invoker.
- `ThreadAttr` в конструкторе `jthread`.
- Минимальная версия C++ — 20.
- Use `requires` clauses instead of `std::enable_if_t` in C++20 jthread constructors.
- Suppress C++20-only `modernize-use-designated-initializations`, `modernize-use-ranges`, and `readability-redundant-typename` checks that fire on existing codebase.
- `paraos::counting_semaphore` и `paraos::binary_semaphore` добавлены рядом с legacy `paraos::SemaphoreCounting` / `paraos::SemaphoreBinary`.
- PC/Unix/Windows используют `std::counting_semaphore` через `port_pc/paraos_semaphore_std.hpp`.
- FreeRTOS использует `xSemaphoreCreateCounting` / `xSemaphoreTake` / `xSemaphoreGive` с `std::chrono` таймаутами.
- Локальные `SleepMs()` в migrated тестах заменены на `paraos::sleep_for(std::chrono::milliseconds)`.
- Гонка в `test_message_multithread_many_producer_many_consumers` устранена вызовом `write.Free()` после неуспешного `TryPush()`.
- Phase 36: legacy Thread/Mutex/Semaphore implementation headers deleted; `ThreadAttr` cleaned; `MutexGuard` rewritten as `std::scoped_lock<paraos::mutex>`.
- Phase 37: `paraos::recursive_mutex` introduced; containers, critical section, socket UDP, FreeRTOS jthread, and extra helpers migrated to std-like primitives.
- Phase 38: legacy `port_tests/test_*.cpp` and `example_*.cpp` files removed; `port_tests/CMakeLists.txt` updated; PC `ctest` passes 58/58.
- Phase 39: all PC/FreeRTOS presets verified; clang-tidy clean; PC test count at 58/58 baseline.

### Pending Todos

None.

### Blockers/Concerns

None.

## Deferred Items

| Category | Item | Status | Deferred At |
|----------|------|--------|-------------|
| CI | Add macOS GitLab CI runner | Deferred | 2026-06-20 |
| Docs | macOS-specific build instructions | Deferred | 2026-06-20 |
| Runtime | FreeRTOS task runtime tests on macOS POSIX simulator | Environment limitation | 2026-06-20 |
| Runtime | Windows jthread runtime verification | No Windows host available | 2026-06-20 |

## Session Continuity

Last session: 2026-06-24T13:30:00.000Z
Stopped at: Phase 39 complete; milestone ready for audit

## Operator Next Steps

- Run milestone audit (`gsd-audit-milestone`).
- Complete milestone (`gsd-complete-milestone`).
- Cleanup (`gsd-cleanup`).
