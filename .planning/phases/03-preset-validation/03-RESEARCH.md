# Phase 3 Research: CMake Preset Validation on macOS

**Date:** 2026-06-20
**Phase:** 3 — Preset Validation
**Researcher:** gsd-plan-phase

## Executive Summary

The project defines 13 CMake presets in `CMakePresets.json`. On the current macOS host:

- PC Clang presets are expected to work when forced to `/usr/bin/clang` and `/usr/bin/clang++` because the default `clang` in `PATH` targets `aarch64-unknown-linux-gnu`.
- PC GCC presets require a native macOS GCC (e.g., Homebrew `gcc-14`). If none is installed, they will fail at configure time.
- FreeRTOS Clang presets use the FreeRTOS POSIX simulator and should build with the same Clang override.
- FreeRTOS GCC presets also require macOS GCC.
- The `*_clang_tidy` presets require `clang-tidy` to be available and may fail if the tidy binary is not from the Xcode toolchain.

## Evidence

### Toolchain inventory

```text
$ clang --version
clang version 22.1.0
Target: aarch64-apple-darwin24.5.0  (when using /usr/bin/clang)

$ /usr/bin/clang --version
Apple clang version 17.0.0 (clang-1700.0.13.3)
Target: arm64-apple-darwin24.5.0
Thread model: posix
InstalledDir: /Library/Developer/CommandLineTools/usr/bin

$ gcc --version
Apple clang version ... (symlink to clang, not GCC)

$ command -v clang-tidy
/opt/homebrew/opt/llvm/bin/clang-tidy  (available)
```

### Preset list

```text
pc_debug_clang
pc_debug_clang_trace
pc_debug_clang_polymorphic
pc_debug_gcc
pc_debug_gcc_trace
pc_debug_gcc_clang_tidy
pc_release_clang
freertos_debug_clang
freertos_debug_gcc
freertos_debug_gcc_trace
freertos_debug_gcc_clang_tidy
freertos_release_clang
freertos_debug_clang_trace
```

## Strategy

For each preset:
1. Remove any stale `build/<preset>` directory to avoid cache contamination.
2. Configure with explicit compiler overrides for Clang presets.
3. Build.
4. Run `ctest` with CI flags.
5. Record success/failure and the exact error for failures.

## Open Questions

1. Does `clang-tidy` work with the AppleClang build of the FreeRTOS port?
2. Does the FreeRTOS POSIX simulator compile on macOS without `timer_create` (which macOS lacks)? The FreeRTOS port may use its own tick implementation, but this needs validation.
3. Should we add a macOS-specific CMake preset or a toolchain file to avoid manual compiler overrides?

## References

- `CMakePresets.json`
- Phase 2 summary (pc_debug_clang passes with `/usr/bin/clang`)
