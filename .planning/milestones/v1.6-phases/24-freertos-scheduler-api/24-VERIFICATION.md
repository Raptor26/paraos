---
phase: 24
status: passed
completed: 2026-06-22
---

# Phase 24 Verification: FreeRTOS scheduler API

## Success Criteria Check

1. ✅ Разработчик может вызвать `paraos::jthread::start_scheduler()` в FreeRTOS-приложении.
   - Added as `public static void start_scheduler()` in `port_freertos/paraos_jthread.hpp`.
2. ✅ `paraos::jthread::is_scheduler_running()` возвращает корректное состояние до и после запуска планировщика.
   - Added as `public static bool is_scheduler_running()` using `xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED`.
3. ✅ `paraos::Thread` не изменился; `paraos::jthread` не использует `paraos::Thread`.
   - No modifications to `port_freertos/paraos_thread.hpp`; no `Thread` references added to `port_freertos/paraos_jthread.hpp`.

## Verification Evidence

- `freertos_debug_clang` configured and built `test_jthread_basic` successfully.
- `freertos_debug_gcc` configured and built `test_jthread_basic` successfully.
- Build log: `.planning/phases/24-freertos-scheduler-api/24-02-build.log`.

## Notes

- Runtime execution of FreeRTOS tests is not available on the macOS POSIX simulator host; validation is build-only.
- `end_scheduler()` returns `bool` per the phase context decision D-04.
