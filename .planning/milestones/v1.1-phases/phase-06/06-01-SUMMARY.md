---
phase: phase-06
plan: "06-01"
subsystem: static-analysis
tags:
  - clang-tidy
  - core-headers
  - port_unix
  - cpp

requires:
  - phase: phase-05
    provides: Complete warning classification log (WARNINGS.md)

provides:
  - True-positive fixes for `readability-use-concise-preprocessor-directives` in core/container/extra headers
  - Documented inline suppressions for intentional `misc-multiple-inheritance`
  - Fix for `readability-redundant-parentheses` in cooperative-scheduling header
  - Verification log proving Phase-6-scoped warnings are gone from the PC tidy preset

affects:
  - phase-07
  - phase-08

tech-stack:
  added: []
  patterns:
    - "Use `#ifdef` / `#ifndef` for single-macro preprocessor conditions"
    - "Wrap intentional multiple-inheritance with `NOLINTBEGIN/NOLINTEND` and a rationale comment"

key-files:
  created:
    - .planning/phases/phase-06/06-01-SUMMARY.md
    - .planning/phases/phase-06/pc_tidy_after_phase6.log
  modified:
    - paraos_config.hpp
    - paraos_exceptions.hpp
    - paraos_thread_common.hpp
    - containers/paraos_queue_blocking.hpp
    - extra/paraos_thread_cooperative_scheduling.hpp

key-decisions:
  - "Replaced single-macro `#if defined(X)` and `#if !defined(X)` with `#ifdef X` / `#ifndef X` where equivalent, leaving compound conditions unchanged."
  - "Suppressed `misc-multiple-inheritance` for `paraos::exception` and `CooperativeScheduling` with inline `NOLINTBEGIN/NOLINTEND` blocks and explicit rationales."
  - "No public API signatures were changed; only preprocessor style and suppression comments were added."

patterns-established:
  - "Inline NOLINT suppression blocks include a rationale comment explaining why the check is a false positive for PARAOS."

requirements-completed:
  - CORE-01
  - CORE-02
  - PORT-01
  - PORT-02
  - PORT-03

# Metrics
duration: 3min
completed: 2026-06-20
---

# Phase 6 Plan 01: Fix Core, Headers & port_unix

**Fixed or documented-suppressed all clang-tidy warnings in library/header scope, preserving public API signatures and platform semantics.**

## Performance

- **Duration:** 3 min
- **Started:** 2026-06-20T16:03:20Z
- **Completed:** 2026-06-20T16:07:09Z
- **Tasks:** 7
- **Files modified:** 5

## Accomplishments
- Replaced 6 single-macro `#if defined(...)` / `#if !defined(...)` conditions with concise `#ifdef` / `#ifndef` forms in core and container headers.
- Added documented inline `NOLINTBEGIN(misc-multiple-inheritance)` / `NOLINTEND(...)` blocks around `paraos::exception` and `CooperativeScheduling`.
- Removed redundant parentheses around `*(scheduled_task)` in `extra/paraos_thread_cooperative_scheduling.hpp`.
- Verified that Phase-6-scoped warnings no longer appear in the `pc_debug_gcc_clang_tidy` build log.
- Confirmed public API signatures are unchanged and Linux/Windows code paths remain semantically identical.

## Task Commits

Each task was committed atomically:

1. **Tasks 1–5 (header fixes and suppressions)** — `6899e59` `fix(06-01): resolve clang-tidy warnings in core and extra headers`
2. **Task 6 (verification log)** — `ba2cd25` `docs(06-01): capture post-fix tidy build log`
3. **Plan metadata** — this file.

## Files Created/Modified
- `paraos_config.hpp` — `#if defined(__clang__)` → `#ifdef __clang__`; `#if defined(_MSC_VER)` → `#ifdef _MSC_VER`.
- `paraos_exceptions.hpp` — `#if defined(PARAOS_VERBOSE_ERRORS)` → `#ifdef PARAOS_VERBOSE_ERRORS`; added documented `misc-multiple-inheritance` suppression around `paraos::exception`.
- `paraos_thread_common.hpp` — two `#if defined(PARAOS_LIKE_WINAPI)` → `#ifdef PARAOS_LIKE_WINAPI`.
- `containers/paraos_queue_blocking.hpp` — `#if !defined(ETL_CHECK_PUSH_POP)` → `#ifndef ETL_CHECK_PUSH_POP`.
- `extra/paraos_thread_cooperative_scheduling.hpp` — removed redundant parentheses; added documented `misc-multiple-inheritance` suppression around `CooperativeScheduling`.
- `.planning/phases/phase-06/pc_tidy_after_phase6.log` — post-fix PC tidy build output.
- `.planning/phases/phase-06/06-01-SUMMARY.md` — this summary.

## Decisions Made
- **Scope discipline:** Left all test/example `.cpp` warnings (internal linkage, static-in-anonymous-namespace, ranges, override visibility) for Phase 7.
- **Suppression style:** Used `NOLINTBEGIN`/`NOLINTEND` blocks for class-level multiple-inheritance warnings so the rationale comment sits directly above the affected class and future additions inside the class remain covered.
- **No `.clang-tidy` changes:** Kept the existing check set intact; all suppressions are inline and rationale-bearing.

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered
- `port_unix/` contains only header files in this repository; the Phase 6 scope effectively narrowed to the header warnings classified in Phase 5, with `port_unix/tests/test_paraos_utils.cpp` deferred to Phase 7 as a test file.
- The PC tidy build still fails after Phase 6 because Phase 7 test/example warnings remain; this is expected and does not block Phase 6 verification.

## User Setup Required
None — no external service configuration required.

## Next Phase Readiness
- Phase 7 can begin fixing all remaining test/example warnings using `WARNINGS.md`.
- No blockers.

---
*Phase: phase-06*
*Completed: 2026-06-20*
