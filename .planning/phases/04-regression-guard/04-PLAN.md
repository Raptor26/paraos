---
phase: 4
phase_name: Regression Guard
plan_id: 04-01
plan_name: Final code review, diff audit, and documentation update
wave: 1
depends_on: []
requirements:
  - REG-01
  - REG-02
  - REG-03
files_modified:
  - .planning/phases/04-regression-guard/04-SUMMARY.md
  - README.md
  - .planning/STATE.md
  - .planning/ROADMAP.md
autonomous: true
gap_closure: false
---

# Plan 04-01: Regression Guard

**Owner:** Implementer
**Estimated effort:** Low
**Input:** Phase 3 summary, full milestone diff, README.md, GitLab CI config
**Output:** Final audit confirming no cross-platform regressions, updated README

## Objective

Review the milestone diff against the GSD project start commit, verify that
Linux/Windows/FreeRTOS code paths are unaffected, ensure all macOS-specific
changes are isolated, and update documentation with the macOS status.

## must_haves

1. Full diff from milestone start (`0680965`) is inspected and classified.
2. All macOS-specific preprocessor branches (`__APPLE__`) are inside `port_unix/`
   except the two documented enablements in core headers.
3. Linux code paths are unchanged under `__linux__`.
4. No Windows or FreeRTOS port files are modified.
5. README.md reflects macOS support and the AppleClang override hint.
6. Phase 4 summary and project tracking files are committed.

## Tasks

### Task 1 — Diff audit

<task type="auto">
  <action>
    Run `git diff 0680965 HEAD --stat` and `git diff 0680965 HEAD --name-only`.
    Separate files into: (a) `port_unix/` implementation, (b) core headers with
    macOS enablement, (c) documentation/planning artifacts. Verify no files in
    `port_win/`, `port_freertos/`, or platform-independent public headers were
    changed except `paraos_check.h` and `paraos_runtime_profiler.hpp`.
  </action>
  <acceptance_criteria>
    - File classification list exists in task notes.
    - No unexpected files outside `port_unix/` are modified.
  </acceptance_criteria>
</task>

### Task 2 — Platform isolation check

<task type="auto">
  <action>
    Search the diff for `__APPLE__`, `__linux__`, `PARAOS_LIKE_UNIX`,
    `PARAOS_LIKE_WINAPI`, and `PARAOS_LIKE_FREERTOS`. Confirm that every
    `__APPLE__` block is either in `port_unix/` or in the two documented
    `#if defined(__unix__) || defined(__APPLE__)` enablements. Confirm that
    every `__linux__` block still contains the original code.
  </action>
  <acceptance_criteria>
    - Platform macro usage is documented in task notes.
    - No macOS branch leaks into Windows/FreeRTOS code paths.
  </acceptance_criteria>
</task>

### Task 3 — Final smoke test

<task type="auto">
  <action>
    Re-run the `pc_debug_clang` preset from a clean build directory using
    AppleClang explicitly, with `--timeout 20`. Confirm 50/50 tests pass.
  </action>
  <acceptance_criteria>
    - Configure, build, and test all return exit code 0.
    - 50/50 tests passed.
  </acceptance_criteria>
</task>

### Task 4 — Documentation update

<task type="auto">
  <action>
    Add a macOS bullet and a short macOS note block to README.md under
    "Supported Operating Systems". Include the AppleClang override command and
    the FreeRTOS POSIX simulator limitation.
  </action>
  <acceptance_criteria>
    - README.md lists macOS as supported.
    - AppleClang override hint is present.
    - FreeRTOS simulator limitation is documented.
  </acceptance_criteria>
</task>

### Task 5 — Summary and tracking update

<task type="auto">
  <action>
    Write `04-SUMMARY.md` with the audit results, the file classification list,
    the smoke-test result, and the README update note. Update `.planning/STATE.md`
    and `.planning/ROADMAP.md` to mark Phase 4 and the milestone complete.
  </action>
  <acceptance_criteria>
    - `04-SUMMARY.md` exists and is committed.
    - `ROADMAP.md` Phase 4 checkbox is complete.
    - `STATE.md` reflects milestone complete.
  </acceptance_criteria>
</task>

## Verification Plan

### Automated / Build

- [ ] `pc_debug_clang` passes from clean build with `--timeout 20`.

### Static / Docs

- [ ] README.md updated.
- [ ] Phase 4 summary committed.

### Manual

- [ ] Diff audit reviewed and signed off in summary.

## Risks and Dependencies

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Unexpected file outside `port_unix/` changed | Low | High | Diff audit catches it |
| README wording drift | Low | Low | Keep note concise |

## Definition of Done

- [ ] All Phase 4 success criteria are met.
- [ ] `04-CONTEXT.md`, `04-PLAN.md`, and `04-SUMMARY.md` are committed.
- [ ] `ROADMAP.md` Phase 4 plan checkbox is complete.
- [ ] `STATE.md` is updated for milestone complete.
- [ ] README.md documents macOS support.

## Artifacts this phase produces

- `.planning/phases/04-regression-guard/04-SUMMARY.md`
- Updated `README.md`
