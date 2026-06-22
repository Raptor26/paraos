---
phase: 24
plan: "24-01"
subsystem: freertos-jthread
tags:
  - freertos
  - jthread
  - scheduler

requires:
  - phase: 23
    provides: Build/tests/static-analysis baseline for v1.5

provides:
  - Static scheduler-control methods in `paraos::jthread` for FreeRTOS
  - `start_scheduler()` with repeated-call guard
  - `is_scheduler_running()` state query
  - `end_scheduler()` safe shutdown helper

affects:
  - 24-02
  - 25-01

tech-stack:
  added: []
  patterns:
    - "Delegate FreeRTOS scheduler API through `paraos::jthread` static methods"
    - "Use `PARAOS_CHECK_ASSERT` for debug-build programming-error guards"

key-files:
  created:
    - .planning/phases/24-freertos-scheduler-api/24-01-PLAN.md
    - .planning/phases/24-freertos-scheduler-api/24-01-SUMMARY.md
  modified:
    - port_freertos/paraos_jthread.hpp

key-decisions:
  - "Repeated calls to `start_scheduler()` are guarded by `PARAOS_CHECK_ASSERT(xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)`."
  - "`end_scheduler()` returns `bool` to signal whether `vTaskEndScheduler()` was actually invoked."
  - "Runtime limitation notes for FreeRTOS POSIX/macOS are documented in Doxygen comments."
  - "No dependency on `paraos::Thread` was introduced."

patterns-established:
  - "Header-only FreeRTOS `jthread` extensions live next to the existing `jthread` class."

requirements-completed:
  - SCHED-01
  - SCHED-02
  - SCHED-03
  - SCHED-07

# Metrics
duration: 5min
completed: 2026-06-22
---

# Phase 24 Plan 01: Add scheduler control methods to FreeRTOS jthread

Implemented the three static scheduler-control methods in `port_freertos/paraos_jthread.hpp`.
