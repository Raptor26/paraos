# Phase 4: Regression Guard — Summary

**Completed:** 2026-06-20
**Phase status:** Complete
**Auditor:** gsd-autonomous / implementer

## Objective

Confirm that the macOS port work does not regress Linux, Windows, or FreeRTOS
builds or semantics, and update public documentation with the macOS status.

## Diff audit

Scope: `git diff 0680965 HEAD` (milestone start to Phase 4 completion).

### File classification

| Category | Files | Notes |
|----------|-------|-------|
| `port_unix/` implementation | `paraos_mutex.hpp`, `paraos_semaphore.hpp`, `paraos_thread.hpp`, `paraos_timer.hpp` | macOS-specific additions isolated behind `#ifdef __APPLE__` / `#elif defined(__linux__)` branches. |
| Core header enablement | `paraos_check.h`, `paraos_runtime_profiler.hpp` | Added `defined(__APPLE__)` to existing Unix-style guards so macOS receives the same behavior as Linux/Unix. No signatures changed. |
| Documentation | `README.md` | Added macOS bullet and note block. |
| Planning artifacts | `.planning/STATE.md`, `.planning/ROADMAP.md`, `.planning/phases/**/*` | GSD workflow artifacts only. |

### Files intentionally not touched

- `port_win/` — no changes.
- `port_freertos/` — no changes.
- `CMakeLists.txt`, `CMakePresets.json`, `.gitlab-ci.yml` — no changes.
- Public API headers (`paraos_*.hpp`) — no signature changes.

### Platform macro isolation

All `__APPLE__` blocks are located in:

- `port_unix/paraos_mutex.hpp`
- `port_unix/paraos_semaphore.hpp`
- `port_unix/paraos_thread.hpp`
- `port_unix/paraos_timer.hpp`

The only other macOS-related change is the addition of `|| defined(__APPLE__)`
to two existing Unix-detecting `#if` guards in `paraos_check.h` and
`paraos_runtime_profiler.hpp`. No `__APPLE__` references leak into Windows or
FreeRTOS code paths.

### Linux path preservation

Every `#ifdef __linux__` branch retains its original implementation. macOS code
was added only via `#elif defined(__APPLE__)` or new `#ifdef __APPLE__` helpers;
no existing Linux code was modified or removed.

## Final smoke test

```bash
rm -rf build/pc_debug_clang
cmake --preset pc_debug_clang \
      -D CMAKE_C_COMPILER=/usr/bin/clang \
      -D CMAKE_CXX_COMPILER=/usr/bin/clang++
cmake --build build/pc_debug_clang
ctest --test-dir build/pc_debug_clang \
      --output-on-failure \
      --stop-on-failure \
      --schedule-random \
      --timeout 20 \
      -j4
```

Result: **50/50 tests passed**.

## Documentation update

`README.md` updated:

- Added **macOS** to the supported operating systems list.
- Added a macOS note block explaining:
  - `port_unix` builds natively on macOS.
  - AppleClang override command for Clang presets on Apple Silicon.
  - FreeRTOS POSIX simulator debug presets are unsupported/hanging on macOS.

## Known limitations (carried forward from Phase 3)

1. FreeRTOS POSIX simulator debug presets hang at runtime on macOS; release
   presets build and have no tests. This is a simulator limitation, not a
   regression in Windows/Linux FreeRTOS behavior.
2. `*_clang_tidy` presets fail on this macOS due to Homebrew LLVM 22.1.7
   surfacing pre-existing warnings in core headers and test files. These
   warnings are not in the macOS-specific `port_unix` changes.

## Success criteria check

| Criterion | Status |
|-----------|--------|
| Linux-specific code paths unchanged under `__linux__` | ✅ |
| No files outside `port_unix/` modified unless documented | ✅ |
| GitLab CI presets remain semantically valid | ✅ (no CI changes) |
| README.md documents macOS support | ✅ |
| Final smoke test passes | ✅ |

## Definition of Done

- [x] Full diff audited and classified.
- [x] Platform isolation verified.
- [x] Final smoke test passed (50/50 tests).
- [x] README.md updated with macOS note.
- [x] `04-SUMMARY.md`, `04-CONTEXT.md`, `04-PLAN.md` committed.
- [x] `ROADMAP.md` and `STATE.md` updated for milestone completion.

---
*Phase: 04-regression-guard*
*Status: COMPLETE — milestone v1.0 ready for archival*
