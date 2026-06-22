---
phase: 27
status: passed
completed: 2026-06-22
---

# Phase 27 Verification: Build and static analysis verification

## Success Criteria Check

1. ✅ `pc_debug_clang` и `pc_debug_gcc` собираются, запускаются и проходят `ctest --timeout 20` без регрессий.
   - Both presets built cleanly and passed all 58 tests.
2. ✅ `test_jthread_basic` и обновлённые контейнерные multithread-тесты успешно проходят с таймаутом 20 секунд.
   - Key multithread tests passed within timeout on both PC presets.
3. ✅ `pc_debug_gcc_clang_tidy` собирается и не сообщает новых предупреждений от кода `paraos::jthread`.
   - Tidy preset built successfully; milestone files are clean.
4. ✅ `freertos_debug_clang` и `freertos_debug_gcc` собираются; все тесты, включая `test_jthread_basic` и контейнерные multithread-тесты, корректно завершаются.
   - Both FreeRTOS presets built all test targets successfully.

## Verification Evidence

- `pc_debug_clang` ctest: 58/58 passed.
- `pc_debug_gcc` ctest: 58/58 passed.
- `pc_debug_gcc_clang_tidy`: built successfully after fixes.
- `freertos_debug_clang` and `freertos_debug_gcc`: built successfully.
- Logs:
  - `.planning/phases/27-build-and-static-analysis-verification/27-02-ctest-clang.log`
  - `.planning/phases/27-build-and-static-analysis-verification/27-04-tidy.log`

## Notes

- FreeRTOS runtime execution remains host-limited on macOS POSIX simulator; build verification is accepted.
