---
phase: 25
plan: "25-02"
subsystem: pc-jthread
tags:
  - pc
  - jthread
  - scheduler

requires:
  - plan: 25-01
    provides: Scheduler state, registry, and per-thread gate

provides:
  - `start_scheduler()` releases waiting threads and sets scheduler state to running
  - `end_scheduler()` stops and joins all active threads and sets scheduler state to stopped
  - `is_scheduler_running()` reports scheduler state
  - Worker threads gate on the per-thread gate before executing user code

affects:
  - 25-03
  - 26-01

tech-stack:
  added: []
  patterns:
    - "Collect registry snapshot under mutex, then operate on snapshot to avoid holding lock during join"
    - "Use atomic exchange for state transitions to make start/end idempotent"

key-files:
  created:
    - .planning/phases/25-pc-scheduler-state-and-gating/25-02-PLAN.md
    - .planning/phases/25-pc-scheduler-state-and-gating/25-02-SUMMARY.md
  modified:
    - port_pc/paraos_jthread.hpp

key-decisions:
  - "Threads created while scheduler is running start immediately; threads created after end_scheduler exit immediately."
  - "end_scheduler() returns false if scheduler was not running."
  - "Destructor releases the private gate with should_run=false to avoid deadlock when destroying a waiting thread."
  - "Doxygen comments warn against calling end_scheduler() from a worker thread."

patterns-established:
  - "Scheduler methods use registry snapshot to stop/join threads without holding registry lock."

requirements-completed:
  - SCHED-04
  - SCHED-05
  - SCHED-06

# Metrics
duration: 20min
completed: 2026-06-22
---

# Phase 25 Plan 02: Wire gating into MakeThread and implement scheduler methods

Connected the gating infrastructure to the thread launch path and implemented
`start_scheduler()`, `end_scheduler()`, and `is_scheduler_running()`.
