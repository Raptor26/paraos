# Phase 3: Preset Validation — Context

**Gathered:** 2026-06-20
**Status:** Ready for planning
**Source:** Auto-generated from ROADMAP phase description and Phase 2 completion state.

## Phase Boundary

Configure, build, and run `ctest` for every available CMake preset on this macOS machine. Document which presets are valid on macOS and capture any failures or toolchain notes.

## Implementation Decisions

### Locked decisions

- Use `cmake --list-presets` to enumerate all presets.
- For each preset, run `cmake --preset <name>`, `cmake --build build/<name>/`, and `ctest --test-dir build/<name>/` with standard CI flags.
- The default `clang`/`clang++` in `PATH` on this machine targets Linux (`aarch64-unknown-linux-gnu`), so presets that use Clang must be configured with `-D CMAKE_C_COMPILER=/usr/bin/clang -D CMAKE_CXX_COMPILER=/usr/bin/clang++`.
- GCC presets (`pc_debug_gcc*`, `freertos_debug_gcc*`) are unlikely to work natively on macOS unless GCC is installed. Document the result without modifying presets.
- FreeRTOS presets compile the FreeRTOS POSIX simulator. Validate them if the toolchain supports it; otherwise record the blocker.

### Claude's discretion

- Determine whether to add a macOS toolchain file or helper script for Clang presets. Default is to document the explicit compiler override rather than change shared presets, unless a minimal change is required.
- Decide how to handle presets that cannot configure on macOS: record them in the phase report with the exact error, do not fail the phase if the failure is due to missing toolchain (e.g., GCC on macOS).

## Canonical References

- `.planning/REQUIREMENTS.md` — VAL-01, VAL-02, VAL-03
- `.planning/ROADMAP.md` — Phase 3 scope
- `.planning/phases/02-verify-behavior-parity/02-SUMMARY.md` — Phase 2 result (pc_debug_clang passes)
- `CMakePresets.json`
- `builder.py` and `pybuilder/pycmakebuilder.py` — CI preset selection logic

## Specific Ideas

- Iterate over presets in groups: PC Clang, PC GCC, FreeRTOS Clang, FreeRTOS GCC.
- For each successful preset, capture `ctest` summary lines.
- For each failed preset, capture the first failing command and its output.
- Update deferred items if macOS build instructions need to be written later.

## Deferred Ideas

- macOS-specific build instructions in `README.md` — deferred to v2 (CI-02).
- Adding macOS GitLab CI runner — deferred to v2 (CI-01).

---

*Phase: 03-preset-validation*
*Context gathered: 2026-06-20*
