# Phase 34: Migrate `extra/tests` standalone executables and CMake - Plan

**Phase:** 34
**Goal:** Standalone multithread tests in `extra/tests/` use the modern `paraos::jthread` lifecycle.
**Strategy:** Verify test sources and update CMake rules.

## Plan

### 1. Verify standalone test sources

- Confirm `extra/tests/test_oneshot_executor_thread.cpp`, `extra/tests/test_paraos_thread_sequence.cpp`, and `extra/tests/test_paraos_cooperative_scheduling_thread.cpp` contain no references to the legacy `paraos::Thread` class (only `paraos::ThreadPriority`, `paraos::ThreadSequence`, `paraos::ThreadSequenceAttr`, `paraos::ThreadAttr` remain).
- Confirm the tests use `paraos::jthread::start_scheduler()` / `end_scheduler()` and local `paraos::jthread` objects for RAII cleanup.

### 2. Update `extra/tests/CMakeLists.txt`

- Change `target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17 c_std_11)` to `cxx_std_20 c_std_11` for the GoogleTest target.
- Add `target_compile_features(... PRIVATE cxx_std_20)` to the three standalone targets (`test_paraos_thread_sequence`, `test_paraos_cooperative_scheduling_thread`, `test_paraos_oneshot_executor`).
- Add `test_paraos_oneshot_executor` to the `CLANG_TIDY_ENABLE` block so it also gets `CXX_CLANG_TIDY`.

### 3. Verify

- Configure and build `pc_debug_clang` preset.
- Run the three standalone tests and `[PARAOS EXTRA]` GoogleTest suite.

## Success Criteria Verification

- [ ] Standalone test sources no longer reference `paraos::Thread`.
- [ ] Standalone tests use `paraos::jthread::start_scheduler()` / `end_scheduler()` and RAII cleanup.
- [ ] `extra/tests/CMakeLists.txt` compiles standalone targets with `cxx_std_20`.
- [ ] `CXX_CLANG_TIDY` attached to all standalone targets when `CLANG_TIDY_ENABLE` is on.
- [ ] All relevant tests pass.
