---
phase: phase-05
plan: "05-01"
subsystem: static-analysis
tags:
  - clang-tidy
  - cmake
  - macos
  - static-analysis

requires:
  - phase: milestone-v1.0
    provides: macOS-compiling PARAOS tree with working CMake presets

provides:
  - Captured `pc_debug_gcc_clang_tidy` build log
  - Captured `freertos_debug_gcc_clang_tidy` build log
  - Deduplicated warning classification log (`WARNINGS.md`)
  - Scoped Phase 6/7 fix plan (core/headers vs. tests/examples)

affects:
  - phase-06
  - phase-07

tech-stack:
  added: []
  patterns:
    - "Use `ninja -k 0` to collect all clang-tidy diagnostics in a single build pass"
    - "Deduplicate warnings by (file, line, check) across translation units"

key-files:
  created:
    - .planning/phases/phase-05/pc_tidy_build_full.log
    - .planning/phases/phase-05/freertos_tidy_build_full.log
    - .planning/phases/phase-05/WARNINGS.md
    - .planning/phases/phase-05/05-01-SUMMARY.md
  modified: []

key-decisions:
  - "Used `ninja -k 0` instead of iteratively suppressing each blocker, because `WarningsAsErrors:'*'` stops the build and the same header warnings repeat across many translation units."
  - "No temporary `// TODO(phase5)` suppressions were added to source files; this phase is purely diagnostic."

patterns-established:
  - "Build-log + parse workflow for static-analysis triage"
  - "True-positive / false-positive / third-party classification with proposed fix phase"

requirements-completed:
  - REPR-01
  - REPR-02

# Metrics
duration: 7min
completed: 2026-06-20
---

# Phase 5 Plan 01: Build `*_clang_tidy` presets and classify all reported warnings

**Complete clang-tidy warning inventory for macOS `*_clang_tidy` presets, with file/line/check classification and fix scope for Phases 6 and 7.**

## Performance

- **Duration:** 7 min
- **Started:** 2026-06-20T13:02:16Z
- **Completed:** 2026-06-20T13:09:23Z
- **Tasks:** 7
- **Files modified:** 0 (diagnostic phase)

## Accomplishments
- Configured and built `pc_debug_gcc_clang_tidy` from a clean state, capturing all diagnostics.
- Configured and built `freertos_debug_gcc_clang_tidy` from a clean state, capturing all diagnostics.
- Parsed and deduplicated warnings by `(file, line, check)` from both logs.
- Created `WARNINGS.md` classifying 56 unique PC warnings, 45 unique FreeRTOS warnings, with 45 overlapping warnings.
- Identified 11 warnings unique to the PC preset (core headers + `port_tests/example_socket_udp.cpp`).
- Scoped fixes: Phase 6 for core headers / `port_unix` / `extra` library code; Phase 7 for tests, examples, and container test helpers.

## Task Commits

Each deliverable was committed atomically:

1. **Task 1-2 (environment + PC configure)** — no separate commit (configuration artifacts are in `build/` which is git-ignored).
2. **Task 3-5 (build logs)** — `3b7c927` `docs(05-01): capture clang-tidy build logs`
3. **Task 6 (classification log)** — `7d09ea6` `docs(05-01): add warning classification log`
4. **Plan metadata** — this file.

## Files Created/Modified
- `.planning/phases/phase-05/pc_tidy_build_full.log` — Full PC tidy build output (`ninja -k 0`).
- `.planning/phases/phase-05/freertos_tidy_build_full.log` — Full FreeRTOS tidy build output (`ninja -k 0`).
- `.planning/phases/phase-05/pc_tidy_configure.log` — PC CMake configure output.
- `.planning/phases/phase-05/freertos_tidy_configure.log` — FreeRTOS CMake configure output.
- `.planning/phases/phase-05/pc_tidy_build.log` — First PC tidy build attempt (without `-k 0`).
- `.planning/phases/phase-05/WARNINGS.md` — Deduplicated warning classification log.
- `.planning/phases/phase-05/05-01-SUMMARY.md` — This summary.

## Decisions Made
- **Deviation:** Used `ninja -k 0` to collect all warnings in one pass rather than iteratively adding `// TODO(phase5)` suppressions. Rationale: `WarningsAsErrors:'*'` stops the build at the first warning, and the same header warnings repeat across dozens of translation units; `-k 0` yields the same classification result far faster.
- Kept changes outside `.planning/` (no source edits) to keep Phase 5 strictly diagnostic.
- Preserved `.clang-tidy` check set; no check categories were disabled.

## Deviations from Plan

### Auto-fixed Issues
None — no implementation work was required.

### Planned approach adjustment
**[Documentation deviation] Used `ninja -k 0` instead of iterative suppress-and-rebuild**
- **Found during:** Task 3 (PC tidy build)
- **Issue:** The plan prescribed iteratively suppressing each blocker to discover the next warning, but the same header warnings (`paraos_config.hpp`, `paraos_exceptions.hpp`, `paraos_thread_common.hpp`, `containers/paraos_queue_blocking.hpp`) are emitted by nearly every translation unit, making iterative rebuilds impractical.
- **Fix:** Ran both builds with `ninja -k 0`, which continues past failures and surfaces every warning that clang-tidy reports for every reachable translation unit.
- **Files modified:** None (only log files created).
- **Verification:** Parsed both logs and confirmed the unique warning counts are stable: 56 PC, 45 FreeRTOS, 45 overlap.
- **Committed in:** `3b7c927` (build logs) and `7d09ea6` (classification log).

---

**Total deviations:** 1 documentation/workflow deviation (no source changes).
**Impact on plan:** No scope creep; classification goal fully achieved.

## Issues Encountered
- **Compiler in PATH targets Linux:** The `clang`/`clang++` first in `PATH` target `aarch64-unknown-linux-gnu`, but the presets use `gcc`/`g++`, which resolve to `/usr/bin/gcc` and `/usr/bin/g++` (Apple Clang 21, target `arm64-apple-darwin25.5.0`). No override was needed.
- **Build does not complete:** Because `WarningsAsErrors:'*'` turns every tidy warning into a hard error, the build cannot succeed until warnings are fixed. The logs still capture every reported warning.
- **Core header warnings only in PC preset:** `paraos_config.hpp`, `paraos_exceptions.hpp`, and `paraos_thread_common.hpp` warnings appeared only in the PC preset log. The FreeRTOS preset built the `paraos` library target and several test targets without surfacing those core header diagnostics. This is noted in `WARNINGS.md` and does not affect Phase 6 scope (those headers are still classified).

## User Setup Required
None — no external service configuration required.

## Next Phase Readiness
- Phase 6 can begin fixing core/header warnings using `WARNINGS.md` as the authoritative input.
- Phase 7 can begin fixing test/example warnings using the same log.
- No blockers.

---
*Phase: phase-05*
*Completed: 2026-06-20*
