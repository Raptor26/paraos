---
gsd_state_version: 1.0
milestone: v1.10
milestone_name: Modernize Unix timer with paraos primitives
current_phase: 41
current_phase_name: core-jthread-based-loop
status: executing
stopped_at: Roadmap created for v1.10 phases 40–44; ready to plan Phase 40
last_updated: "2026-06-25T16:22:52.455Z"
last_activity: 2026-06-25
last_activity_desc: Phase 41 execution started
progress:
  total_phases: 5
  completed_phases: 1
  total_plans: 2
  completed_plans: 1
  percent: 20
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-06-25)

**Core value:** Кроссплатформенная переносимость PARAOS сохраняется: код, работающий на Linux/Windows/FreeRTOS, продолжает работать, а новая macOS-разработка ведётся на равных с остальными платформами, включая статический анализ clang-tidy.
**Current focus:** Phase 41 — core-jthread-based-loop

## Current Position

Phase: 41 (core-jthread-based-loop) — EXECUTING
Plan: 1 of 1
Status: Executing Phase 41
Last activity: 2026-06-25 — Phase 41 execution started

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
- Milestone v1.10 started: goal is to replace POSIX/pthread timer implementation in `port_unix/paraos_timer.hpp` with `paraos::jthread`, `paraos::mutex`, `paraos::binary_semaphore`, and `paraos::sleep_for`.

### Pending Todos

- Phase 40: create header skeleton, add `port_tests/test_timer.cpp`, wire CMake.

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

Last session: 2026-06-25T15:55:12.453Z
Stopped at: Roadmap created for v1.10 phases 40–44; ready to plan Phase 40

## Operator Next Steps

- Plan Phase 40 (design and test scaffold) via `/gsd-plan-phase 40` or equivalent.
