# PARAOS

## What This Is

PARAOS — это C++ слой абстракции ОС (OSAL) для встраиваемых систем, оборачивающий примитивы Windows, Linux и FreeRTOS так, чтобы один и тот же прикладной код мог нативно запускаться на ПК для тестирования и на целевом устройстве в продакшене. Веха v1.0 добавила поддержку macOS, а веха v1.1 обеспечила прохождение `*_clang_tidy` CMake-пресетов на macOS без регрессий на других платформах.

## Core Value

Кроссплатформенная переносимость PARAOS сохраняется: код, работающий на Linux/Windows/FreeRTOS, продолжает работать, а новая macOS-разработка ведётся на равных с остальными платформами, включая статический анализ clang-tidy.

## Current State

**Shipped:** v1.1 Static Analysis Cleanup (2026-06-20)

- `*_clang_tidy` пресеты (`pc_debug_gcc_clang_tidy`, `freertos_debug_gcc_clang_tidy`) собираются на macOS без предупреждений clang-tidy.
- Публичное API PARAOS не изменено.
- `port_win/` и `port_freertos/` не затронуты; Linux-пути оставлены семантически неизменными.
- Все 50 тестов CTest проходят для `pc_debug_gcc_clang_tidy`.
- Не-tidy пресеты (`pc_debug_gcc`, `freertos_debug_gcc`) продолжают собираться.

## Next Milestone Goals

**Кандидаты для v1.2:**

- CI-01: добавить GitLab CI job для macOS-раннера.
- CI-02: добавить GitLab CI job для `*_clang_tidy` пресетов на macOS.
- DOCS-01: документировать macOS-специфичные инструкции по сборке в `README.md`.
- DOCS-02: задокументировать политику статического анализа и правила добавления суппрессий.

Финальный выбор следующей вехи определяется через `/gsd-new-milestone`.

## Requirements

### Validated

- ✓ Все исходники `port_unix/` компилируются на macOS с дефолтным Clang — v1.0 Phase 1.
- ✓ Отсутствующие POSIX API (`timer_create`, `timer_t`, `itimerspec`) заменены на macOS-совместимые — v1.0 Phase 1.
- ✓ Платформенные отличия изолированы через `__APPLE__` / `__linux__` — v1.0 Phase 1.
- ✓ Публичные примитивы PARAOS ведут себя одинаково на Linux и macOS — v1.0 Phase 2.
- ✓ Все доступные CMake-пресеты на macOS конфигурируются и собираются — v1.0 Phase 3.
- ✓ Нет регрессий для Linux, Windows и FreeRTOS — v1.0 Phase 4.
- ✓ Все предупреждения clang-tidy из `*_clang_tidy` пресетов на macOS классифицированы в `.planning/phases/phase-05/WARNINGS.md` — v1.1 Phase 5.
- ✓ `*_clang_tidy` пресеты успешно конфигурируются и собираются на macOS — v1.1 Phase 6.
- ✓ Публичное API PARAOS осталось неизменным — v1.1 Phase 6.
- ✓ Предупреждения в тестах и примерах исправлены или документированы — v1.1 Phase 7.
- ✓ Регрессионная защита пройдена — v1.1 Phase 8.

### Active

- [ ] CI-01: добавить GitLab CI job для macOS-раннера.
- [ ] CI-02: добавить GitLab CI job для `*_clang_tidy` пресетов на macOS.
- [ ] DOCS-01: документировать macOS-специфичные инструкции по сборке в `README.md`.
- [ ] DOCS-02: задокументировать политику статического анализа и правила добавления суппрессий.

### Out of Scope

- Добавление новых портов или платформ — текущий фокус на macOS-хосте, не расширение списка портов.
- Оптимизация runtime-производительности вне необходимого для сборки.
- Изменение публичного API или заголовочной поверхности PARAOS.
- Тестирование на физических целевых устройствах — только host-сборки.
- Глобальное переписывание CI/CD вне явно выделенных CI-задач следующей вехи.

## Context

- PARAOS — библиотека только из исходников; потребители подключают через `add_subdirectory`.
- `.clang-tidy` настроен широким набором проверок и `WarningsAsErrors: '*'`, поэтому любое предупреждение ломает сборку.
- Веха v1.0 оставила `*_clang_tidy` пресеты падающими на macOS из-за прежних предупреждений в core и тестах.
- macOS — основная машина разработки; валидация других платформ обеспечивается изоляцией изменений и сохранением существующих платформенных путей.

## Constraints

- **Стек**: C++17, CMake ≥ 3.20, Clang/GCC/MSVC, GoogleTest, clang-tidy.
- **Статический анализ**: набор проверок `.clang-tidy` сохраняется; суппрессии требуют явного обоснования.
- **Безопасность платформ**: изменения не должны затрагивать поведение или сборку `port_win/` и `port_freertos/`.
- **Минимальные изменения**: предпочтение точечным исправлениям и inline-суппрессиям вместо отключения целых категорий.
- **Покрытие тестами**: после очистки все доступные пресеты на этой машине должны проходить `ctest`.

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Keep changes inside `port_unix` with macro isolation | Preserves existing port architecture and minimizes review surface | ✓ Good |
| Use macOS-native substitutes for missing POSIX timers | `timer_create` is not implemented on macOS | ✓ Good |
| Validate every available CMake preset on this machine | Ensures the fix does not silently break Clang/GCC or FreeRTOS PC simulation presets | ✓ Good |
| Preserve `.clang-tidy` check set and only add documented suppressions | Keeps the project's static-analysis bar high while removing false positives | ✓ Good |
| Use `ninja -k 0` for warning classification | `WarningsAsErrors:'*'` останавливает сборку; `-k 0` собирает все диагностики за один проход | ✓ Good |
| Inline `NOLINTBEGIN/NOLINTEND` with rationale comments | Обоснование сидит рядом с классом, будущие изменения внутри класса остаются покрыты | ✓ Good |

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
*Last updated: 2026-06-20 after v1.1 milestone completed*
