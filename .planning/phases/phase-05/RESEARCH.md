# Phase 5 Research: clang-tidy Presets on macOS

**Date:** 2026-06-20
**Phase:** 5 — Reproduce & Classify clang-tidy warnings
**Researcher:** gsd-plan-phase

## Executive Summary

The PARAOS v1.1 milestone must make the two `*_clang_tidy` CMake presets pass on macOS. This phase only reproduces and classifies the warnings; later phases fix them. The project already integrates clang-tidy at the CMake target level, uses a broad `.clang-tidy` check set with `WarningsAsErrors: '*'`, and applies clang-tidy to the main library and to selected test/example targets. On this machine clang-tidy is available via Homebrew (`/usr/local/bin/clang-tidy`), and both tidy presets have been configured before, but their warning output must be captured cleanly from scratch and classified.

## clang-tidy Presets

`CMakePresets.json` defines exactly two tidy presets:

| Preset | Base preset | Extra cache variable | Port selected |
|--------|-------------|----------------------|---------------|
| `pc_debug_gcc_clang_tidy` | `pc_debug_gcc` | `CLANG_TIDY_ENABLE=true` | `port_unix/` (`PARAOS_LIKE_UNIX`) |
| `freertos_debug_gcc_clang_tidy` | `freertos_debug_gcc` | `CLANG_TIDY_ENABLE=true` | `port_freertos/` (`PARAOS_LIKE_FREERTOS`) |

Both presets use GCC (`gcc`/`g++`) as the compiler and run `clang-tidy` as a post-compile static-analysis step. Because the host is macOS, `port_unix/` is the PC port that participates in the `pc_debug_gcc_clang_tidy` build. The FreeRTOS preset compiles the POSIX simulator port inside `port_freertos/FreeRTOS-Kernel/portable/ThirdParty/GCC/Posix/` plus PARAOS FreeRTOS wrappers.

## How clang-tidy Is Wired

The top-level `CMakeLists.txt` enables clang-tidy on the `paraos` static-library target:

```cmake
if(IS_PARAOS_STAND_ALONE_PROJECT AND CLANG_TIDY_ENABLE)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}")
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
  else()
    message(FATAL_ERROR "clang-tidy not found.")
  endif()
endif()
```

Test/example directories add clang-tidy individually to the targets they own:

- `port_tests/CMakeLists.txt` — `test_paraos_core`, `example_thread_check_timeout`, `example_timer`, `test_thread_only_*`, etc.
- `containers/tests/CMakeLists.txt` — container stress and unit tests.
- `extra/tests/CMakeLists.txt` — `test_paraos_extra`, `test_paraos_thread_sequence`, `test_paraos_cooperative_scheduling_thread`, etc.

`port_unix/tests/CMakeLists.txt` does **not** mention `CLANG_TIDY_ENABLE`, so `test_paraos_unix` is not currently analyzed. The plan should verify whether it should be added, or document that it is intentionally excluded.

## `.clang-tidy` Configuration

The project-wide config (`.clang-tidy`) enables many check categories and treats every warning as an error:

```yaml
Checks: >-
  clang-analyzer-*,
  google-*,
  hicpp-*,
  llvm-*,
  misc-*,
  -misc-include-cleaner,
  modernize-*,
  performance-*,
  portability-*,
  readability-*,
  -readability-avoid-const-params-in-decls,
  -llvm-header-guard,
  -modernize-avoid-c-arrays,
  -readability-redundant-access-specifiers,
  -readability-implicit-bool-conversion,
  -hicpp-static-assert,
  -misc-static-assert
WarningsAsErrors: '*'
HeaderFilterRegex: '.*\.hpp'
FormatStyle: file
```

Key implications:

- Any warning from an enabled check fails the build.
- Header files matching `*.hpp` are also analyzed when included by compiled translation units.
- The check set must be preserved (requirement SUPP-02); only documented false-positive suppressions may be added.

## Files Expected to Produce Warnings

Based on the preset scopes and the fact that v1.0 left tidy presets failing, warnings are expected in at least these areas:

### `pc_debug_gcc_clang_tidy`

- Public core headers (`paraos_*.hpp`, `paraos_*.h`) included by `paraos` and tests.
- `port_unix/` sources (the macOS-compatible code added in v1.0 is new and untidy).
- `port_tests/` sources and executable examples enabled for clang-tidy.
- `containers/tests/`, `extra/tests/`, and possibly `port_unix/tests/` sources.

### `freertos_debug_gcc_clang_tidy`

- Public core headers again (shared with the PC build).
- FreeRTOS-Kernel POSIX simulator sources (`port_freertos/FreeRTOS-Kernel/portable/ThirdParty/GCC/Posix/`).
- PARAOS FreeRTOS wrappers (`port_freertos/`).
- The same test/example set as the PC preset, because tests are still built when `RTOS_NAME=FREERTOS`.

## macOS Toolchain Considerations

- The `clang` first in `PATH` targets Linux (`aarch64-unknown-linux-gnu`). Presets use `gcc`/`g++`, which on this machine must resolve to a macOS-targeting GCC or be overridden.
- Phase 1 established the workaround: override compilers explicitly when configuring, e.g. `-D CMAKE_C_COMPILER=/usr/bin/clang -D CMAKE_CXX_COMPILER=/usr/bin/clang++`. For the GCC presets the same principle applies — use the compiler that produces a macOS binary.
- `clang-tidy` itself is from Homebrew LLVM 22.1.7 and is independent of the compiler choice.

## Classification Methodology

Each warning will be recorded with:

1. **Source file** (relative path).
2. **Line and column**.
3. **Check name** (e.g., `cppcoreguidelines-avoid-magic-numbers`, `readability-named-parameter`).
4. **Warning message** (short summary).
5. **Classification**:
   - **True positive** — code violates the intent of the check and should be fixed.
   - **False positive** — check misfires on PARAOS-specific patterns; should be suppressed with rationale.
   - **Third-party** — warning originates in a vendored dependency (`etl/`, `leaf/`, `GSL/`, `lwrb/`, `FreeRTOS-Kernel/`); note whether it is filtered by `HeaderFilterRegex` or target-level exclusion.
6. **Proposed action** — fix location, inline `// NOLINT(...)`/`NOLINTNEXTLINE(...)`, or add a targeted `.clang-tidy` rule with rationale.

## Open Questions to Resolve During Execution

1. Are the same warnings repeated across the two presets, or does each preset surface unique issues?
2. Does `HeaderFilterRegex: '.*\.hpp'` cause warnings in vendored `.hpp` headers that should instead be excluded?
3. Are there warnings in `port_unix/tests/` that should be captured by enabling clang-tidy there?
4. Does the FreeRTOS POSIX simulator port produce warnings that PARAOS should fix, suppress, or exclude as third-party code?

## References

- `.clang-tidy`
- `CMakePresets.json`
- `CMakeLists.txt` (lines 120–130)
- `port_tests/CMakeLists.txt` (lines 118+)
- `containers/tests/CMakeLists.txt` (lines 80+)
- `extra/tests/CMakeLists.txt` (lines 52+)
- `.planning/REQUIREMENTS.md` (REPR-01, REPR-02)
- `.planning/ROADMAP.md` (Phase 5)
