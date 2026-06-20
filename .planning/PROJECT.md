# PARAOS: Static Analysis Cleanup

## What This Is

PARAOS is a C++ OS abstraction layer (OSAL) for embedded systems that wraps Windows, Linux, and FreeRTOS primitives so the same application code can run natively on PC for testing and on target in production. After fixing macOS compilation in v1.0, this milestone makes the `*_clang_tidy` CMake presets pass on macOS by cleaning up all clang-tidy warnings they report, while keeping every other platform's build and behavior intact.

## Core Value

All `*_clang_tidy` CMake presets configure, build, and pass `ctest` on the current macOS hardware without regressing any other platform's build or behavior.

## Current Milestone: v1.1 Static Analysis Cleanup

**Goal:** Make `*_clang_tidy` CMake presets pass on macOS by fixing or suppressing clang-tidy warnings, without changing the public API or `.clang-tidy` check set beyond documented false-positive suppressions.

**Target features:**
- Reproduce and classify all clang-tidy warnings reported by `*_clang_tidy` presets on macOS.
- Fix warnings in core headers, `port_unix/`, tests, and other files participating in macOS presets.
- Suppress documented false positives in `.clang-tidy` or inline when fixing is impossible or undesirable.
- Preserve Linux, Windows, and FreeRTOS build behavior and semantics.
- Do not modify GitLab CI pipeline configuration.

## Requirements

### Validated

- All `port_unix/` sources compile cleanly on macOS with the default Clang toolchain — v1.0 Phase 1.
- Missing POSIX APIs (`timer_create`, `timer_t`, `itimerspec`) replaced with macOS-compatible equivalents — v1.0 Phase 1.
- Platform-specific differences isolated using `__APPLE__` / `__linux__` preprocessor macros — v1.0 Phase 1.
- Public OSAL primitives behave identically on Linux and macOS — v1.0 Phase 2.
- Every available CMake preset on macOS configures and builds — v1.0 Phase 3.
- No regressions introduced for Linux, Windows, or FreeRTOS builds — v1.0 Phase 4.
- All clang-tidy warnings reported by `*_clang_tidy` presets on macOS are classified in `.planning/phases/phase-05/WARNINGS.md` — Phase 5.

### Active

<!-- Current scope. Building toward these. -->

- [x] All `*_clang_tidy` presets configure and build successfully on macOS — CORE-01/CORE-02/PORT-01 in Phase 6
- [ ] All clang-tidy warnings that break macOS presets are fixed or documented as false positives
- [x] Public API surface of PARAOS remains unchanged — Phase 6
- [x] Linux, Windows, and FreeRTOS builds and semantics are not regressed — Phase 6
- [x] `.clang-tidy` check set is preserved; only false-positive suppressions are added — preserved through Phases 5 and 6

### Out of Scope

<!-- Explicit boundaries. Includes reasoning to prevent re-adding. -->

- Adding new ports or platforms — this milestone is about static analysis, not port expansion.
- Runtime performance optimization beyond what is needed for tidy-clean compilation.
- Changing public API or header surface of PARAOS.
- Physical target/embedded testing — only host PC builds.
- Modifying GitLab CI pipeline configuration or adding new CI jobs.

## Context

- PARAOS is a source-only CMake library; consumers add it via `add_subdirectory`.
- `.clang-tidy` is configured with a broad check set and `WarningsAsErrors: '*'`, so every warning fails the build.
- The v1.0 milestone left `*_clang_tidy` presets failing on macOS due to pre-existing warnings in core and test files.
- macOS is the primary development machine for this cleanup; validation of other platforms is by construction — keeping changes isolated and preserving existing platform-specific code paths.

## Constraints

- **Tech stack**: C++17, CMake ≥ 3.20, Clang/GCC/MSVC, GoogleTest, clang-tidy
- **Static analysis**: `.clang-tidy` check set must be preserved; suppressions need explicit rationale
- **Platform safety**: Changes must not modify `port_win/` or `port_freertos/` behavior or builds
- **Minimal change**: Prefer targeted fixes and inline suppressions over broad rule disables
- **Test coverage**: After cleanup, all available presets on this machine must still pass `ctest` with standard flags

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Keep changes inside `port_unix` with macro isolation | Preserves existing port architecture and minimizes review surface | ✓ Good |
| Use macOS-native substitutes for missing POSIX timers (`dispatch`, `kqueue`, or `pthread` timed waits) | `timer_create` is not implemented on macOS | ✓ Good |
| Validate every available CMake preset on this machine | Ensures the fix does not silently break Clang/GCC or FreeRTOS PC simulation presets | ✓ Good |
| Preserve `.clang-tidy` check set and only add documented suppressions | Keeps the project's static-analysis bar high while removing false positives | — Pending |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-06-20 after Phase 5 completed*
