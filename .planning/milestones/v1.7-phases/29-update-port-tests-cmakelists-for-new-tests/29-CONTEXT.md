# Phase 29 Context: Update `port_tests/CMakeLists.txt` for new tests

**Phase:** 29  
**Name:** Update `port_tests/CMakeLists.txt` for new tests  
**Milestone:** v1.7  
**Discussed:** 2026-06-23

## Domain

Формальное обновление `port_tests/CMakeLists.txt` для четырёх мигрированных standalone-тестов `test_thread_only_*`. Phase 28 уже выполнила миграцию исходников; Phase 28 также временно переключила цели на `cxx_std_20` для верификации сборки. Phase 29 закрепляет это формально и добавляет недостающие элементы: регистрацию в `CXX_CLANG_TIDY` и таймаут CTest 20 секунд.

## Locked Decisions

1. **cxx_std_20**: четыре цели `test_thread_only_*` уже используют `cxx_std_20` (изменение внесено в Phase 28 для верификации и сохраняется).
2. **CXX_CLANG_TIDY**: блок `if(CLANG_TIDY_ENABLE)` уже включает четыре цели `test_thread_only_*`; проверить, что регистрация корректна после переименования/миграции.
3. **CTest TIMEOUT 20**: каждый из четырёх `add_test(...)` должен иметь `TIMEOUT 20`.

## Canonical Refs

- `.planning/REQUIREMENTS.md` — BUILD-01..BUILD-04.
- `.planning/ROADMAP.md` — Phase 29 success criteria.
- `port_tests/CMakeLists.txt` — файл для изменения.

## Code Context

Текущий `port_tests/CMakeLists.txt`:
- `target_compile_features(test_thread_only_* PRIVATE cxx_std_20 c_std_11)` — уже установлено.
- В `if(CLANG_TIDY_ENABLE)` уже есть `set_target_properties(test_thread_only_* PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")`.
- `add_test` для четырёх целей не имеет `TIMEOUT`.

## Deferred Ideas

None.

## Open Questions

None.
