# Phase 30 Context: Runtime verification on macOS

**Phase:** 30  
**Name:** Runtime verification on macOS  
**Milestone:** v1.7  
**Discussed:** 2026-06-23

## Domain

Запуск всех тестов на macOS для PC-пресетов и FreeRTOS-пресетов с таймаутом 20 секунд. Цель — убедиться, что мигрированные `test_thread_only_*` проходят в рамках CTest и не вызывают зависаний.

## Locked Decisions

1. PC-пресеты (`pc_debug_clang`, `pc_debug_gcc`) запускаются через `ctest --timeout 20`.
2. FreeRTOS-пресеты (`freertos_debug_clang`, `freertos_debug_gcc`) также запускаются через `ctest --timeout 20`.
3. Все четыре `test_thread_only_*` теста должны завершиться успешно.
4. Кроссплатформенность проверяется через отсутствие изменений в платформенно-специфичных путях Windows/Linux (изменения ограничены `port_tests/test_thread_only_*.cpp` и `port_tests/CMakeLists.txt`).

## Canonical Refs

- `.planning/REQUIREMENTS.md` — TEST-01..TEST-03.
- `.planning/ROADMAP.md` — Phase 30 success criteria.
- `port_tests/CMakeLists.txt` — CTest регистрация с TIMEOUT 20.

## Code Context

После Phase 28 и Phase 29:
- Четыре теста переписаны на `paraos::jthread`.
- CMake настроен на `cxx_std_20`, `CXX_CLANG_TIDY` и `TIMEOUT 20`.
- PC и FreeRTOS сборки уже успешно собирались.

## Deferred Ideas

None.

## Open Questions

None.
