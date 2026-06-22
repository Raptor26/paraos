---
wave: 1
depends_on: "22"
files_modified:
  - .planning/ROADMAP.md
  - .planning/STATE.md
  - containers/paraos_ringbuff.hpp (if pre-existing tidy warnings are fixed)
  - .planning/phases/23-build-tests-static-analysis/23-SUMMARY.md
  - .planning/phases/23-build-tests-static-analysis/23-VERIFICATION.md
autonomous: true
---

# Phase 23: Build, tests and static analysis - Plan

**Plan ID:** 23-01
**Phase:** 23
**Created:** 2026-06-22
**Status:** Ready for execution
**Mode:** Standard
**Context:** `.planning/phases/23-build-tests-static-analysis/23-CONTEXT.md`

## Goal

Verify that all v1.5 changes build cleanly, pass tests, and do not introduce new static-analysis warnings on the full preset matrix.

## Scope

- Run full `cmake --build` and `ctest` for `pc_debug_clang` and `pc_debug_gcc`.
- Verify compilation for `freertos_debug_clang` and `freertos_debug_gcc`.
- Run `pc_debug_gcc_clang_tidy` and `freertos_debug_gcc_clang_tidy` presets; fix any new warnings from changed files and root-cause/fix pre-existing warnings that break the build.
- Run stress tests (`ctest -L stress --repeat-until-fail 100`) on PC Clang and GCC.
- Document results and update project state.

## Requirements Addressed

- **BLD-01**: PC presets build and pass `ctest`.
- **BLD-02**: FreeRTOS presets compile without new errors.
- **BLD-03**: clang-tidy presets produce no new warnings from changed files.
- **BLD-04**: Stress tests remain available and pass for migrated multithread tests.

## Out of Scope

- FreeRTOS runtime execution on hardware/simulator.
- Windows runtime verification.
- macOS POSIX timer fixes unrelated to v1.5.

## Research Summary

Phase 22 already established the verification baseline. No additional research is required.

## Tasks

### Task 1: Full PC Clang build and test

<action>
```bash
cmake --build build/pc_debug_clang/
ctest --test-dir build/pc_debug_clang/ --output-on-failure --timeout 60
```
</action>

<acceptance_criteria>
- Build completes without errors.
- All tests pass.
</acceptance_criteria>

### Task 2: Full PC GCC build and test

<action>
```bash
cmake --build build/pc_debug_gcc/
ctest --test-dir build/pc_debug_gcc/ --output-on-failure --timeout 60
```
</action>

<acceptance_criteria>
- Build completes without errors.
- All tests pass.
</acceptance_criteria>

### Task 3: FreeRTOS compilation

<action>
```bash
cmake --build build/freertos_debug_clang/
cmake --build build/freertos_debug_gcc/
```
</action>

<acceptance_criteria>
- Both presets compile without new errors.
</acceptance_criteria>

### Task 4: clang-tidy preset build and warning triage

<action>
```bash
cmake --build build/pc_debug_gcc_clang_tidy/
cmake --build build/freertos_debug_gcc_clang_tidy/
```
</action>

<acceptance_criteria>
- No new warnings from files modified in Phases 19-22.
- Pre-existing warnings that break the build are root-caused and fixed or documented.
</acceptance_criteria>

### Task 5: Stress tests

<action>
```bash
ctest --test-dir build/pc_debug_clang/ -L stress --output-on-failure --timeout 30 --repeat-until-fail 100
ctest --test-dir build/pc_debug_gcc/ -L stress --output-on-failure --timeout 30 --repeat-until-fail 100
```
</action>

<acceptance_criteria>
- 100% pass rate on both PC Clang and GCC.
</acceptance_criteria>

### Task 6: Documentation and state update

<action>
- Create `23-SUMMARY.md` and `23-VERIFICATION.md`.
- Update `.planning/STATE.md` and `.planning/ROADMAP.md` to mark Phase 23 complete and milestone v1.5 shipped.
</action>

<acceptance_criteria>
- Phase 23 artifacts exist.
- STATE.md reflects Phase 23 complete.
- ROADMAP.md marks v1.5 shipped.
</acceptance_criteria>

## Verification

### Static / Review

- [ ] All changed files from Phases 19-22 are accounted for.
- [ ] No new clang-tidy warnings originate from changed files.
- [ ] Documentation is updated.

### Automated

- [ ] PC Clang full test suite passes.
- [ ] PC GCC full test suite passes.
- [ ] FreeRTOS Clang and GCC compile.
- [ ] clang-tidy presets build (or pre-existing warnings are documented).
- [ ] Stress tests pass 100 repetitions on PC Clang and GCC.

## Definition of Done

- [ ] All PC presets build and pass `ctest`.
- [ ] FreeRTOS presets compile.
- [ ] clang-tidy presets produce no new warnings from changed files.
- [ ] Stress tests pass.
- [ ] `23-SUMMARY.md` and `23-VERIFICATION.md` are created.
- [ ] `.planning/STATE.md` and `.planning/ROADMAP.md` are updated.
- [ ] Phase 23 is committed.

## Artifacts this phase produces

- Planning artifacts:
  - `.planning/phases/23-build-tests-static-analysis/23-SUMMARY.md`
  - `.planning/phases/23-build-tests-static-analysis/23-VERIFICATION.md`
- Possible code fix:
  - `containers/paraos_ringbuff.hpp` if pre-existing tidy warnings are fixed.

## must_haves

### truths
- PC Clang/GCC full test suites pass.
- FreeRTOS Clang/GCC presets compile.
- clang-tidy presets have no new warnings from v1.5 changed files.
- Stress tests are reliable under repetition.

### prohibitions
- statement: Do not suppress or ignore root-caused warnings without fixing them or documenting why they are deferred.
  status: resolved
  verification: Every clang-tidy warning is classified as "new from changed files", "pre-existing and fixed", or "pre-existing and documented".
