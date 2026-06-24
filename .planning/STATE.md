---
gsd_state_version: 1.0
milestone: v1.9
milestone_name: "Remove legacy Thread/Mutex/Semaphore implementations"
current_phase: 37
status: executing
stopped_at: Phase 36 complete; executing Phase 37
last_updated: "2026-06-24T12:00:00.000Z"
last_activity: 2026-06-24
last_activity_desc: Phase 36 complete — legacy headers removed
progress:
  total_phases: 4
  completed_phases: 1
  total_plans: 4
  completed_plans: 1
  percent: 25
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-06-24)

**Core value:** Кроссплатформенная переносимость PARAOS сохраняется: код, работающий на Linux/Windows/FreeRTOS, продолжает работать, а новая macOS-разработка ведётся на равных с остальными платформами, включая статический анализ clang-tidy.
**Current focus:** Milestone v1.9 — remove legacy Thread/Mutex/Semaphore implementations.

## Current Position

Phase: 37 — Migrate internal consumers to std-like primitives
Plan: 37-01
Status: Executing
Last activity: 2026-06-24 — Phase 36 complete; legacy headers removed

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
- Phase 36: legacy Thread/Mutex/Semaphore implementation headers deleted; `ThreadAttr` cleaned; `MutexGuard` rewritten as `std::lock_guard<paraos::mutex>`.
- Phase 37: introduce `paraos::recursive_mutex` for `CriticalSection` because `paraos::mutex` is non-recursive.

### Pending Todos

- Migrate `containers/` to std-like mutex/semaphore.
- Migrate `port_unix/paraos_critical.hpp` to `paraos::recursive_mutex`.
- Migrate socket UDP headers from `paraos::Thread::DelayMs` to `paraos::sleep_for`.

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

Last session: 2026-06-24T12:00:00.000Z
Stopped at: Phase 36 complete; executing Phase 37

## Operator Next Steps

- Execute Phase 37 plan 37-01.
