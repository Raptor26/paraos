# Roadmap: PARAOS Static Analysis Cleanup

## Overview

Make the `*_clang_tidy` CMake presets pass on macOS by reproducing, classifying, and fixing all clang-tidy warnings they report, while preserving the existing `.clang-tidy` check set and without regressing other platforms.

## Phases

- [x] **Phase 5: Reproduce & Classify clang-tidy warnings** — Configure and build every `*_clang_tidy` preset on macOS; capture and classify all warnings (completed 2026-06-20)
- [x] **Phase 6: Fix Core, Headers & port_unix** — Clean up warnings in public core headers and `port_unix/` sources (completed 2026-06-20)
- [x] **Phase 7: Fix Tests, Examples & Document Suppressions** — Clean up warnings in tests and examples; document false-positive suppressions (completed 2026-06-20)
- [x] **Phase 8: Regression Guard** — Verify non-tidy presets still build and other platforms are untouched (completed 2026-06-20)

## Phase Details

### Phase 5: Reproduce & Classify clang-tidy warnings

**Goal**: Every `*_clang_tidy` preset on macOS is built and every reported warning is classified.
**Depends on**: Milestone v1.0 complete
**Requirements**: REPR-01, REPR-02
**Success Criteria**:

  1. `pc_debug_gcc_clang_tidy` configures and builds to completion (capturing warnings)
  2. `freertos_debug_gcc_clang_tidy` configures and builds to completion (capturing warnings)
  3. A classification log exists listing each warning: file, line, check, true/false positive, proposed action

**Plans**: 1 plan

Plans:

- [x] 05-01: Build `*_clang_tidy` presets and classify all reported warnings

### Phase 6: Fix Core, Headers & port_unix

**Goal**: clang-tidy warnings in public core headers and `port_unix/` sources are fixed or documented as false positives.
**Depends on**: Phase 5
**Requirements**: CORE-01, CORE-02, PORT-01, PORT-02, PORT-03
**Success Criteria**:

  1. No clang-tidy warnings remain in `paraos_*.hpp` / `paraos_*.h` files, or each remaining warning has an inline suppression with rationale
  2. No clang-tidy warnings remain in `port_unix/` sources, or each remaining warning has an inline suppression with rationale
  3. Public header signatures are unchanged
  4. Linux-specific code paths under `__linux__` are semantically unchanged

**Plans**: 1 plan

Plans:

- [x] 06-01: Fix or suppress clang-tidy warnings in core headers and `port_unix/`

### Phase 7: Fix Tests, Examples & Document Suppressions

**Goal**: clang-tidy warnings in tests, examples, and other ancillary files are fixed or documented as false positives.
**Depends on**: Phase 6
**Requirements**: TEST-01, TEST-02, SUPP-01, SUPP-02
**Success Criteria**:

  1. No clang-tidy warnings remain in `port_tests/`, `containers/tests/`, `extra/tests/`, `port_unix/tests/`, or executable examples
  2. Test logic and assertions are preserved
  3. Every `.clang-tidy` or inline suppression has a documented rationale
  4. The existing `.clang-tidy` check set is preserved

**Plans**: 1 plan

Plans:

- [x] 07-01: Fix or suppress clang-tidy warnings in tests and examples; document suppression rationale

### Phase 8: Regression Guard

**Goal**: Static-analysis cleanup does not regress non-tidy builds or other platforms.
**Depends on**: Phase 7
**Requirements**: REG-01, REG-02, REG-03
**Success Criteria**:

  1. `pc_debug_clang` and `pc_debug_gcc` presets configure and build successfully
  2. No files outside `port_unix/` are modified except core headers/tests required for tidy cleanup
  3. `port_win/` and `port_freertos/` files are unchanged in behavior and build semantics
  4. A final diff audit confirms platform isolation

**Plans**: 1 plan

Plans:

- [x] 08-01: Final diff audit and non-tidy preset smoke test

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 5. Reproduce & Classify clang-tidy warnings | 1/1 | Complete    | 2026-06-20 |
| 6. Fix Core, Headers & port_unix | 1/1 | Complete    | 2026-06-20 |
| 7. Fix Tests, Examples & Document Suppressions | 1/1 | Complete | 2026-06-20 |
| 8. Regression Guard | 1/1 | Complete | 2026-06-20 |
