---
phase: 3
phase_name: Preset Validation
plan_id: 03-01
plan_name: Configure, build, and test every available CMake preset on macOS
wave: 1
depends_on: []
requirements:
  - VAL-01
  - VAL-02
  - VAL-03
files_modified:
  - .planning/phases/03-preset-validation/03-SUMMARY.md
  - CMakePresets.json (only if a minimal macOS fix is required and documented)
autonomous: true
gap_closure: false
---

# Plan 03-01: Validate Every CMake Preset on macOS

**Owner:** Implementer
**Estimated effort:** Medium
**Input:** `CMakePresets.json`, current toolchain state on macOS, Phase 2 passing `pc_debug_clang`
**Output:** A documented validation matrix showing which presets configure/build/test on macOS

## Objective

Run `cmake --preset`, `cmake --build`, and `ctest` for every preset listed by `cmake --list-presets`. Record success/failure for each. Apply minimal, documented fixes only if a preset fails for a reason that can be corrected without breaking other platforms.

## must_haves

1. Every preset is attempted.
2. Clang presets use `/usr/bin/clang` and `/usr/bin/clang++` to avoid the Linux-targeting `clang` first in `PATH`.
3. GCC presets are attempted only if a native macOS GCC is available; failures due to missing GCC are documented, not fixed.
4. FreeRTOS presets are attempted; any macOS-specific issues are documented.
5. No preset is modified unless the change is required for macOS and preserves Linux/Windows/FreeRTOS behavior.
6. A validation matrix is produced in `03-SUMMARY.md`.

## Tasks

### Task 1 — Enumerate presets and establish the validation harness

<task type="auto">
  <read_first>
    - CMakePresets.json
    - pybuilder/pycmakebuilder.py
  </read_first>
  <action>
    Run `cmake --list-presets` and capture the full list. Create a small shell loop or Python helper that, for each preset, removes `build/<preset>`, configures with explicit compiler overrides when the preset name contains "clang", builds, runs `ctest --test-dir build/<preset> --output-on-failure --stop-on-failure --schedule-random --timeout 20 -j4`, and records configure/build/test exit codes plus the last 20 lines of any failure. Store the raw output in a temporary file under `.planning/phases/03-preset-validation/`.
  </action>
  <acceptance_criteria>
    - `cmake --list-presets` output is captured in the task notes.
    - A repeatable validation script/loop exists and has been run at least once.
    - Initial results for `pc_debug_clang` show configure 0, build 0, ctest 0 with 50/50 tests passed.
  </acceptance_criteria>
</task>

### Task 2 — Validate PC Clang presets

<task type="auto">
  <read_first>
    - CMakePresets.json
    - .planning/phases/03-preset-validation/03-RESEARCH.md
  </read_first>
  <action>
    For each PC Clang preset (`pc_debug_clang`, `pc_debug_clang_trace`, `pc_debug_clang_polymorphic`, `pc_release_clang`), configure with `-D CMAKE_C_COMPILER=/usr/bin/clang -D CMAKE_CXX_COMPILER=/usr/bin/clang++`, build, and run tests. Record the result. If a preset fails and the fix is a minimal macOS-only change (e.g., a compiler flag or a missing `#ifdef __APPLE__`), apply it, commit, and re-run. Do not modify shared Linux/Windows code paths.
  </action>
  <acceptance_criteria>
    - Each PC Clang preset has a recorded configure/build/test status.
    - All PC Clang presets that can succeed on this macOS with available toolchains do succeed.
    - Any failures are documented with the exact error in the task notes.
  </acceptance_criteria>
</task>

### Task 3 — Validate PC GCC presets

<task type="auto">
  <read_first>
    - CMakePresets.json
  </read_first>
  <action>
    For each PC GCC preset (`pc_debug_gcc`, `pc_debug_gcc_trace`, `pc_debug_gcc_clang_tidy`), attempt configure/build/test using the system's `gcc`/`g++`. If no native macOS GCC is installed and configure fails with "compiler not found" or an AppleClang masquerading as GCC error, record the failure as "macOS GCC not available" and stop. Do not install toolchains or modify presets to fix this.
  </action>
  <acceptance_criteria>
    - Each PC GCC preset has a recorded status.
    - Failures are classified as either "toolchain missing" or "real build issue".
  </acceptance_criteria>
</task>

### Task 4 — Validate FreeRTOS presets

<task type="auto">
  <read_first>
    - CMakePresets.json
    - port_freertos/CMakeLists.txt (if exists)
  </read_first>
  <action>
    For each FreeRTOS preset (`freertos_debug_clang`, `freertos_debug_gcc`, `freertos_debug_gcc_trace`, `freertos_debug_gcc_clang_tidy`, `freertos_release_clang`, `freertos_debug_clang_trace`), attempt configure/build/test. Use explicit `/usr/bin/clang`/`/usr/bin/clang++` for Clang presets. Record results. If FreeRTOS POSIX simulator fails due to missing POSIX timers on macOS, investigate whether the FreeRTOS port already abstracts the tick source and document the finding.
  </action>
  <acceptance_criteria>
    - Each FreeRTOS preset has a recorded status.
    - Any macOS-specific FreeRTOS issues are documented.
  </acceptance_criteria>
</task>

### Task 5 — Produce the validation matrix and summary

<task type="auto">
  <read_first>
    - .planning/phases/03-preset-validation/03-CONTEXT.md
    - .planning/phases/03-preset-validation/03-RESEARCH.md
  </read_first>
  <action>
    Write `03-SUMMARY.md` containing a table with columns: Preset, Configure, Build, Tests, Notes. Mark each cell with ✅/❌/⏭ (not applicable / toolchain missing). Include a short paragraph explaining the macOS toolchain situation and any minimal fixes applied. Update `STATE.md` and `ROADMAP.md` to mark Phase 3 complete.
  </action>
  <acceptance_criteria>
    - `03-SUMMARY.md` exists and contains a complete validation matrix.
    - `ROADMAP.md` Phase 3 checkbox is marked complete.
    - `STATE.md` reflects Phase 3 complete and readiness for Phase 4.
  </acceptance_criteria>
</task>

## Verification Plan

### Automated / Build

- [ ] Every preset in `cmake --list-presets` is attempted.
- [ ] Successful presets pass `ctest` with standard CI flags.

### Static / Docs

- [ ] Validation matrix is accurate and committed.
- [ ] Any preset modifications are documented and preserve cross-platform semantics.

### Manual

- [ ] Review whether failures are due to missing toolchain (acceptable) or real port bugs (must fix or document).

## Risks and Dependencies

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| GCC not installed on macOS | High | Low | Document as toolchain missing; do not fail phase |
| FreeRTOS POSIX simulator uses `timer_create` | Medium | Medium | Investigate FreeRTOS tick source; document blocker |
| Clang-tidy version incompatibility | Medium | Low | Use available `clang-tidy`; document if it fails |
| Preset modification scope creep | Low | High | Only apply minimal macOS-only fixes with full documentation |

## Definition of Done

- [ ] All Phase 3 success criteria are met.
- [ ] `03-CONTEXT.md`, `03-RESEARCH.md`, `03-VALIDATION.md`, and `03-PLAN.md` are committed.
- [ ] `03-SUMMARY.md` is committed with the validation matrix.
- [ ] `ROADMAP.md` Phase 3 plan checkbox is complete.
- [ ] `STATE.md` is updated for Phase 3 complete.

## Artifacts this phase produces

- `.planning/phases/03-preset-validation/03-SUMMARY.md` — validation matrix.
- Optional minimal fixes to `CMakePresets.json` or port code (documented).
