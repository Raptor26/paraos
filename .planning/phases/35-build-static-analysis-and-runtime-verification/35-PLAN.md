# Phase 35: Build, static analysis and runtime verification - Plan

**Phase:** 35
**Goal:** All PC and FreeRTOS presets remain green after the migration.
**Strategy:** Configure, build, and test each relevant preset; fix any issues.

## Plan

### 1. PC presets

1. `pc_debug_clang`
   - Already verified in Phases 31-34.
   - Run `ctest --test-dir build/pc_debug_clang/ --output-on-failure --schedule-random --timeout 20`.
2. `pc_debug_gcc`
   - Configure: `cmake --preset pc_debug_gcc`.
   - Build: `cmake --build build/pc_debug_gcc/ -j4`.
   - Test: `ctest --test-dir build/pc_debug_gcc/ --output-on-failure --schedule-random --timeout 20`.
3. `pc_debug_gcc_clang_tidy`
   - Configure: `cmake --preset pc_debug_gcc_clang_tidy`.
   - Build: `cmake --build build/pc_debug_gcc_clang_tidy/ -j4`.
   - Capture clang-tidy output for modified `extra/` files and fix new warnings.
   - Test: `ctest --test-dir build/pc_debug_gcc_clang_tidy/ --output-on-failure --schedule-random --timeout 20`.

### 2. FreeRTOS presets (compile-only)

1. `freertos_debug_clang`
   - Configure: `cmake --preset freertos_debug_clang`.
   - Build: `cmake --build build/freertos_debug_clang/ -j4`.
2. `freertos_debug_gcc`
   - Configure: `cmake --preset freertos_debug_gcc`.
   - Build: `cmake --build build/freertos_debug_gcc/ -j4`.

### 3. Baseline check

- Confirm PC `ctest` count is at least 58 tests (pre-milestone baseline).

## Success Criteria Verification

- [ ] `pc_debug_clang` configures, builds, and passes `ctest`.
- [ ] `pc_debug_gcc` configures, builds, and passes `ctest`.
- [ ] `pc_debug_gcc_clang_tidy` configures, builds, passes `ctest`, and emits no new clang-tidy warnings in modified `extra/` files.
- [ ] `freertos_debug_clang` compiles.
- [ ] `freertos_debug_gcc` compiles.
- [ ] PC `ctest` count ≥ 58.
