---
phase: 26
plan: "26-01"
subsystem: pc-jthread
tags:
  - pc
  - jthread
  - scheduler

requires:
  - plan: 25-03
    provides: PC scheduler gating implemented

provides:
  - PC end_scheduler() skips self-join
  - Unified test_jthread_basic.cpp without std::_Exit or platform branches

affects:
  - 26-02

tech-stack:
  added: []
  patterns:
    - "Worker thread can call end_scheduler() safely because self-join is skipped"

key-files:
  created:
    - .planning/phases/26-test-unification/26-01-SUMMARY.md
  modified:
    - port_pc/paraos_jthread.hpp
    - port_tests/test_jthread_basic.cpp

key-decisions:
  - "Self-join skip uses std::this_thread::get_id() compared to ctx->thread.get_id()."
  - "Main does not call join() on individual threads; it waits for a shutdown signal from the stopper."

patterns-established:
  - "Cross-platform test pattern: create workers + stopper, start_scheduler, wait for shutdown signal."

requirements-completed:
  - TEST-01
  - TEST-02
  - TEST-04

# Metrics
duration: 25min
completed: 2026-06-22
---

# Phase 26 Plan 01: Update PC end_scheduler and rewrite test_jthread_basic

Implemented self-skip in PC end_scheduler() and rewrote test_jthread_basic.cpp
as the reference unified test.
