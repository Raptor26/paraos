# Phase 5 Plan: Reproduce & Classify clang-tidy warnings

**Phase:** 5 — Reproduce & Classify clang-tidy warnings
**Goal:** Every `*_clang_tidy` preset on macOS is built and every reported warning is classified.
**Depends on:** Milestone v1.0 complete
**Requirements:** REPR-01, REPR-02
**Mode:** Standard
**Research:** `.planning/phases/phase-05/RESEARCH.md`

## Success Criteria

1. `pc_debug_gcc_clang_tidy` configures and builds to completion on macOS, capturing the full clang-tidy warning output.
2. `freertos_debug_gcc_clang_tidy` configures and builds to completion on macOS, capturing the full clang-tidy warning output.
3. A classification log exists listing each warning with file, line, check name, true/false-positive classification, and proposed action.
4. No `.clang-tidy` check categories are disabled during this phase.

## Research Summary

- Two presets enable clang-tidy: `pc_debug_gcc_clang_tidy` and `freertos_debug_gcc_clang_tidy`.
- clang-tidy is applied to the `paraos` library target and to selected test/example targets; `port_unix/tests/` is not currently covered.
- `.clang-tidy` enables a broad check set and `WarningsAsErrors: '*'`, so every warning fails the build.
- On this machine the default `clang` targets Linux; compiler overrides from Phase 1 may be needed for configuration.
- Warnings are expected in public core headers, `port_unix/` sources, tests/examples, and possibly FreeRTOS POSIX simulator sources.

See full findings in `.planning/phases/phase-05/RESEARCH.md`.

---

## Plan 05-01: Build `*_clang_tidy` presets and classify all reported warnings

**Owner:** Implementer
**Estimated effort:** Medium
**Input:** `.clang-tidy`, `CMakePresets.json`, source tree, macOS host
**Output:** `.planning/phases/phase-05/WARNINGS.md` classification log; clean configure/build evidence

### Task 1 — Prepare the build environment

**Scope:** Ensure the machine can run both tidy presets from a clean state.

**Details:**

1. Verify `clang-tidy` is in `PATH`:
   ```bash
   clang-tidy --version
   ```
2. Verify a macOS-targeting C/C++ compiler is available. If the default `gcc`/`g++` in `PATH` targets Linux, use explicit overrides or put `/usr/bin` first in `PATH`.
3. Remove any stale build directories for the two presets to avoid stale `compile_commands.json` or cached results:
   ```bash
   rm -rf build/pc_debug_gcc_clang_tidy
   rm -rf build/freertos_debug_gcc_clang_tidy
   ```

**Constraints:**

- Do not modify `CMakePresets.json` or `.clang-tidy` in this task.
- Use the same compiler for both presets to keep the comparison fair.

### Task 2 — Configure `pc_debug_gcc_clang_tidy`

**Scope:** Produce a clean configure for the PC tidy preset.

**Details:**

1. Configure the preset:
   ```bash
   cmake --preset pc_debug_gcc_clang_tidy
   ```
   If the default `gcc`/`g++` do not target macOS, override explicitly, for example:
   ```bash
   cmake --preset pc_debug_gcc_clang_tidy \
     -D CMAKE_C_COMPILER=/usr/bin/clang \
     -D CMAKE_CXX_COMPILER=/usr/bin/clang++
   ```
2. Confirm `compile_commands.json` is generated in `build/pc_debug_gcc_clang_tidy/`.
3. If configuration fails, capture the error, determine whether it is a compiler-target issue or a CMake logic issue, and record it in the classification log.

**Constraints:**

- Only compiler overrides are allowed; do not change build type or enable/disable other features.

### Task 3 — Build `pc_debug_gcc_clang_tidy` and capture warnings

**Scope:** Run the build and capture all clang-tidy diagnostics.

**Details:**

1. Run the build with output captured:
   ```bash
   cmake --build build/pc_debug_gcc_clang_tidy/ 2>&1 | tee .planning/phases/phase-05/pc_tidy_build.log
   ```
2. Because `WarningsAsErrors: '*'` is set, the build stops at the first warning. For each failure:
   - Record the file, line, column, check name, and message.
   - Apply a minimal local fix or inline suppression to let the build proceed (only if it does not change semantics; otherwise stop and document).
   - Re-run the build.
3. Continue until the build either completes or the remaining warnings are fully classified.
4. Keep the final `pc_tidy_build.log` containing the complete sequence of warnings.

**Constraints:**

- Do not perform broad fixes in this phase; only unblock the build enough to discover the next warning.
- Any inline suppression added to unblock discovery must include a `// TODO(phase5)` marker so Phase 6 can revisit it.

### Task 4 — Configure `freertos_debug_gcc_clang_tidy`

**Scope:** Produce a clean configure for the FreeRTOS tidy preset.

**Details:**

1. Configure the preset:
   ```bash
   cmake --preset freertos_debug_gcc_clang_tidy
   ```
   Apply the same compiler override as Task 2 if needed.
2. Confirm `compile_commands.json` is generated in `build/freertos_debug_gcc_clang_tidy/`.
3. Record any configure failure separately from the PC preset.

### Task 5 — Build `freertos_debug_gcc_clang_tidy` and capture warnings

**Scope:** Run the build and capture all clang-tidy diagnostics for the FreeRTOS preset.

**Details:**

1. Run the build with output captured:
   ```bash
   cmake --build build/freertos_debug_gcc_clang_tidy/ 2>&1 | tee .planning/phases/phase-05/freertos_tidy_build.log
   ```
2. Follow the same iterative record/unblock/iterate process as Task 3.
3. Pay special attention to warnings originating in `port_freertos/FreeRTOS-Kernel/portable/ThirdParty/GCC/Posix/` and decide whether they are third-party warnings.
4. Keep the final `freertos_tidy_build.log`.

### Task 6 — Parse and classify all warnings

**Scope:** Convert the raw build logs into a structured classification log.

**Details:**

1. Create `.planning/phases/phase-05/WARNINGS.md` with the following sections:
   - `## Summary` — total warnings per preset, unique files, check categories.
   - `## pc_debug_gcc_clang_tidy` — one entry per warning.
   - `## freertos_debug_gcc_clang_tidy` — one entry per warning.
   - `## Cross-preset overlap` — warnings that appear in both presets (usually shared core headers).
   - `## Third-party / excluded` — warnings in vendored code that should not be fixed by PARAOS.
2. Each warning entry must include:
   - `File:` relative path.
   - `Line:` line number.
   - `Check:` clang-tidy check name.
   - `Message:` one-line summary.
   - `Classification:` `true-positive`, `false-positive`, or `third-party`.
   - `Proposed action:` fix, inline suppress with rationale, or exclude target/header.
3. Deduplicate warnings that are emitted multiple times for the same line/check (e.g., from multiple translation units including the same header).

**Classification rules:**

- **True positive:** the code genuinely violates the check's intent and can be changed without altering behavior or public API.
- **False positive:** the check misfires on an OSAL idiom (e.g., ISR helper, platform macro, delegate signature). Must be accompanied by a rationale.
- **Third-party:** the warning is in `etl/`, `leaf/`, `GSL/`, `lwrb/`, or `FreeRTOS-Kernel/` and should not be fixed in this milestone. Note if the current setup already filters it.

### Task 7 — Sanity-check coverage

**Scope:** Confirm the classification log is complete enough for Phase 6 to start.

**Details:**

1. Verify that every file mentioned in the logs belongs to one of the expected scopes:
   - `paraos_*.hpp` / `paraos_*.h`
   - `port_unix/`
   - `port_tests/`, `containers/tests/`, `extra/tests/`, `port_unix/tests/`
   - `port_freertos/` (FreeRTOS preset only)
   - vendored dependencies
2. If a warning appears in an unexpected file (e.g., generated `paraos_version.hpp`), note it specially.
3. Confirm that the total warning count matches the number of build-stop events in the logs.

---

## Risks and Dependencies

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Compiler target mismatch prevents preset configuration | Medium | High | Use explicit `CMAKE_C_COMPILER`/`CMAKE_CXX_COMPILER` overrides; document the exact command |
| `WarningsAsErrors: '*'` stops the build after the first warning, hiding later issues | High | Medium | Iteratively fix or `// TODO(phase5)`-suppress each warning to discover the next; rerun until the log is stable |
| Same header warning is emitted from many translation units, inflating the log | Medium | Low | Deduplicate by file/line/check in `WARNINGS.md`; count unique occurrences |
| Vendored FreeRTOS POSIX simulator code produces warnings | Medium | Medium | Classify as third-party unless the warning is in a PARAOS wrapper; do not modify FreeRTOS-Kernel sources without explicit rationale |
| Classification disagrees between implementer and reviewer | Low | High | Require rationale for every false-positive; leave TODO markers on temporary suppressions |

## Verification Plan

### Automated / Build

- [ ] `cmake --preset pc_debug_gcc_clang_tidy` configures successfully.
- [ ] `cmake --build build/pc_debug_gcc_clang_tidy/` produces a captured log.
- [ ] `cmake --preset freertos_debug_gcc_clang_tidy` configures successfully.
- [ ] `cmake --build build/freertos_debug_gcc_clang_tidy/` produces a captured log.

### Static / Logs

- [ ] `.planning/phases/phase-05/pc_tidy_build.log` exists and contains all warnings/errors from the PC tidy build.
- [ ] `.planning/phases/phase-05/freertos_tidy_build.log` exists and contains all warnings/errors from the FreeRTOS tidy build.
- [ ] `.planning/phases/phase-05/WARNINGS.md` exists and classifies every unique warning.
- [ ] No whole-category `.clang-tidy` disables were added in this phase.

### Manual

- [ ] Review `WARNINGS.md` for completeness and consistent classification rationale.
- [ ] Confirm compiler override commands are recorded so Phase 6 can reproduce the same environment.
- [ ] Verify that any temporary inline suppressions are marked `// TODO(phase5)`.

## Definition of Done

- [ ] All Phase 5 success criteria are met.
- [ ] `RESEARCH.md`, `PLAN.md`, and `WARNINGS.md` are committed to `.planning/phases/phase-05/`.
- [ ] Both tidy preset build logs are committed to `.planning/phases/phase-05/`.
- [ ] `WARNINGS.md` contains a complete, deduplicated classification of all warnings.
- [ ] `ROADMAP.md` Phase 5 plan checkbox `05-01` is marked complete.
- [ ] `STATE.md` is updated to reflect Phase 5 complete / Phase 6 ready.

## Notes for Phase 6

- Phase 6 will fix or document-suppress all true-positive and agreed false-positive warnings in public core headers and `port_unix/`.
- Any `// TODO(phase5)` markers introduced to unblock discovery should be removed or converted to permanent, rationale-bearing suppressions by Phase 6.
- The `WARNINGS.md` log is the authoritative input for Phase 6 task prioritization.
