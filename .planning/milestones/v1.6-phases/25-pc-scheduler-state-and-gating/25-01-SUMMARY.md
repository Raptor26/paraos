---
phase: 25
plan: "25-01"
subsystem: pc-jthread
tags:
  - pc
  - jthread
  - scheduler

requires:
  - plan: 24-01
    provides: FreeRTOS scheduler method signatures

provides:
  - Scheduler state enum and atomic state variable
  - Static registry of active jthread contexts
  - Per-thread gate (mutex + condition_variable) in heap-allocated Context
  - Refactored jthread to own a `std::unique_ptr<Context>`

affects:
  - 25-02
  - 25-03

tech-stack:
  added: []
  patterns:
    - "Heap-allocated Context gives a stable pointer for the worker lambda and registry"
    - "Per-thread gate allows individual release in destructor and collective release by scheduler methods"

key-files:
  created:
    - .planning/phases/25-pc-scheduler-state-and-gating/25-01-PLAN.md
    - .planning/phases/25-pc-scheduler-state-and-gating/25-01-SUMMARY.md
  modified:
    - port_pc/paraos_jthread.hpp

key-decisions:
  - "Context is heap-allocated so its address is stable across jthread move operations."
  - "Registry stores Context* and does not need updates on move construction/assignment because the pointer is stable."
  - "Move assignment removes the old Context from the registry before replacing the unique_ptr."
  - "Default constructor added to match std::jthread and represent empty moved-from state."

patterns-established:
  - "PC jthread mirrors FreeRTOS jthread design: Context* + registry + scheduler methods."

requirements-completed:
  - SCHED-04
  - SCHED-05
  - SCHED-06

# Metrics
duration: 15min
completed: 2026-06-22
---

# Phase 25 Plan 01: Add SchedulerState, registry, and per-thread gate to PC jthread

Laid the scheduler-gating foundation in `port_pc/paraos_jthread.hpp`.
