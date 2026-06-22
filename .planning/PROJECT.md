# PARAOS

## What This Is

PARAOS — это C++ слой абстракции ОС (OSAL) для встраиваемых систем, оборачивающий примитивы Windows, Linux и FreeRTOS так, чтобы один и тот же прикладной код мог нативно запускаться на ПК для тестирования и на целевом устройстве в продакшене.

- Веха v1.0 добавила поддержку macOS.
- Веха v1.1 обеспечила прохождение `*_clang_tidy` CMake-пресетов на macOS без регрессий на других платформах.
- Веха v1.2 добавила новый публичный API `paraos::jthread` в стиле `std::jthread`, единый для Windows, Unix и FreeRTOS, рядом с существующим `paraos::Thread`.
- Веха v1.3 добавила новый публичный API `paraos::mutex` в стиле `std::mutex` для PC и FreeRTOS, рядом с существующим `paraos::Mutex`.

## Core Value

Кроссплатформенная переносимость PARAOS сохраняется: код, работающий на Linux/Windows/FreeRTOS, продолжает работать, а новая macOS-разработка ведётся на равных с остальными платформами, включая статический анализ clang-tidy.

## Current Milestone: TBD

**Goal:** Определяется через `/gsd-new-milestone`.

## Current State

**Shipped:** v1.4 std::semaphore-style Semaphore API (2026-06-22)

- Добавлены `paraos::counting_semaphore<LeastMaxValue>` и `paraos::binary_semaphore`.
- PC, Unix и Windows используют `std::counting_semaphore` через `port_pc/paraos_semaphore_std.hpp`.
- FreeRTOS реализован поверх `xSemaphoreCreateCounting` / `xSemaphoreTake` / `xSemaphoreGive`.
- Обеспечена совместимость с `std::chrono` таймаутами (`try_acquire_for`, `try_acquire_until`).
- Добавлен тест `port_tests/test_semaphore_std.cpp` и зарегистрирован в CTest.
- PC-пресеты собираются и проходят `ctest` (55/55); `*_clang_tidy` пресеты без новых предупреждений.

**Shipped:** v1.3 std::mutex-style Mutex API (2026-06-22)

- Добавлен `paraos::mutex` с `lock()`, `try_lock()`, `unlock()` для PC (`port_pc/` → `port_unix/`, `port_win/`) и FreeRTOS (`port_freertos/`).
- PC-порт реализован как тонкая обёртка над `std::mutex`.
- FreeRTOS-порт реализован поверх FreeRTOS mutex API.
- Обеспечена совместимость со `std::lock_guard<paraos::mutex>` и `std::unique_lock<paraos::mutex>`.
- Добавлен тест `port_tests/test_mutex_basic.cpp` и зарегистрирован в CTest.
- PC-пресеты (`pc_debug_clang`, `pc_debug_gcc`) собираются и проходят `ctest` (54/54).
- `freertos_debug_clang` и `freertos_debug_gcc` собираются; `test_mutex_basic` проходит с учётом ограничений POSIX-порта macOS.
- `*_clang_tidy` пресеты не получили новых предупреждений от кода мьютекса.

<details>
<summary>Previous: v1.2 std::jthread-style Thread API (2026-06-20)</summary>

- Добавлен `paraos::jthread`, `paraos::stop_token`, `paraos::stop_source` для PC (`port_pc/` → `port_unix/`, `port_win/`) и FreeRTOS (`port_freertos/`).
- PC-порт реализован как тонкая обёртка над `std::jthread`.
- FreeRTOS-порт реализован поверх FreeRTOS API с поддержкой capturing lambdas через heap-allocated invoker.
- Поддержан `ThreadAttr` (имя, стек, приоритет) в конструкторе `jthread` для всех портов.
- Минимальная версия C++ повышена до 20.
- Добавлен тест `port_tests/test_jthread_basic.cpp` и зарегистрирован в CTest.
- PC-пресеты (`pc_debug_clang`, `pc_debug_gcc`, `pc_debug_gcc_clang_tidy`) собираются и проходят `ctest`.
- `*_clang_tidy` пресеты продолжают собираться без новых предупреждений.
- FreeRTOS runtime-тесты с созданием задач на macOS зависят от POSIX-порта и не выполняются на этом хосте (известное ограничение среды).

</details>

**Previously shipped:** v1.2 std::jthread-style Thread API (2026-06-20)

- Добавлен `paraos::jthread`, `paraos::stop_token`, `paraos::stop_source` для PC (`port_pc/` → `port_unix/`, `port_win/`) и FreeRTOS (`port_freertos/`).
- PC-порт реализован как тонкая обёртка над `std::jthread`.
- FreeRTOS-порт реализован поверх FreeRTOS API с поддержкой capturing lambdas через heap-allocated invoker.
- Поддержан `ThreadAttr` (имя, стек, приоритет) в конструкторе `jthread` для всех портов.
- Минимальная версия C++ повышена до 20.
- Добавлен тест `port_tests/test_jthread_basic.cpp` и зарегистрирован в CTest.
- PC-пресеты (`pc_debug_clang`, `pc_debug_gcc`, `pc_debug_gcc_clang_tidy`) собираются и проходят `ctest`.
- `*_clang_tidy` пресеты продолжают собираться без новых предупреждений.
- FreeRTOS runtime-тесты с созданием задач на macOS зависят от POSIX-порта и не выполняются на этом хосте (известное ограничение среды).

## Requirements

### Validated

- ✓ Все исходники `port_unix/` компилируются на macOS с дефолтным Clang — v1.0 Phase 1.
- ✓ Отсутствующие POSIX API (`timer_create`, `timer_t`, `itimerspec`) заменены на macOS-совместимые — v1.0 Phase 1.
- ✓ Платформенные отличия изолированы через `__APPLE__` / `__linux__` — v1.0 Phase 1.
- ✓ Публичные примитивы PARAOS ведут себя одинаково на Linux и macOS — v1.0 Phase 2.
- ✓ Все доступные CMake-пресеты на macOS конфигурируются и собираются — v1.0 Phase 3.
- ✓ Нет регрессий для Linux, Windows и FreeRTOS — v1.0 Phase 4.
- ✓ Все предупреждения clang-tidy из `*_clang_tidy` пресетов на macOS классифицированы — v1.1 Phase 5.
- ✓ `*_clang_tidy` пресеты успешно конфигурируются и собираются на macOS — v1.1 Phase 6.
- ✓ Публичное API PARAUS осталось неизменным — v1.1 Phase 6.
- ✓ Предупреждения в тестах и примерах исправлены или документированы — v1.1 Phase 7.
- ✓ Регрессионная защита пройдена — v1.1 Phase 8.
- ✓ JT-01: добавить `paraos::jthread` и `paraos::stop_token` в `port_pc/paraos_jthread.hpp` — v1.2 Phase 9.
- ✓ JT-02: добавить `port_unix/paraos_jthread.hpp`, включающий `port_pc/paraos_jthread.hpp` — v1.2 Phase 9.
- ✓ JT-03: добавить `port_win/paraos_jthread.hpp`, включающий `port_pc/paraos_jthread.hpp` — v1.2 Phase 9.
- ✓ JT-04: добавить `paraos::jthread` в `port_freertos/paraos_jthread.hpp` — v1.2 Phase 10.
- ✓ JT-05: поддержать `ThreadAttr` в конструкторе `jthread` — v1.2 Phase 11.
- ✓ JT-06: обеспечить единый пользовательский API без платформенных `#ifdef` — v1.2 Phases 9–11.
- ✓ JT-07: обновить CMake до C++20 — v1.2 Phase 12.
- ✓ JT-08: добавить тест `port_tests/test_jthread_basic.cpp` — v1.2 Phase 12.
- ✓ JT-09: пройти сборку и тесты на PC; FreeRTOS runtime ограничен POSIX-портом macOS — v1.2 Phase 12.
- ✓ JT-10: обеспечить прохождение `*_clang_tidy` пресетов без регрессий — v1.2 Phase 12.
- ✓ MUTEX-01..03: `paraos::mutex` предоставляет `lock()`, `try_lock()`, `unlock()` — v1.3 Phase 13.
- ✓ MUTEX-04: `paraos::mutex` не копируется и не перемещается — v1.3 Phase 13.
- ✓ MUTEX-05..06: совместимость со `std::lock_guard` и `std::unique_lock` — v1.3 Phase 15.
- ✓ MUTEX-07..09: PC-реализация и forwarding-заголовки — v1.3 Phase 13.
- ✓ MUTEX-10: FreeRTOS-реализация — v1.3 Phase 14.
- ✓ MUTEX-11: пользовательский API без платформенных `#ifdef` — v1.3 Phase 13.
- ✓ BUILD-01: `paraos_mutex_std.hpp` доступен из всех портов — v1.3 Phase 13.
- ✓ TEST-01..04: `test_mutex_basic.cpp` компилируется и проходит на PC/FreeRTOS — v1.3 Phase 15.
- ✓ TEST-05: `*_clang_tidy` пресеты без новых предупреждений от кода мьютекса — v1.3 Phase 15.

### Active

_None — start the next milestone with `/gsd-new-milestone`._

### Out of Scope

- Полная замена существующего `paraos::Mutex` на `paraos::mutex` — веха v1.3 добавляет новый API рядом со старым.
- Миграция `extra/` и `containers/` на `paraos::mutex` — откладывается на будущие вехи.
- Добавление `std::recursive_mutex`-подобного API или таймаутов (`try_lock_for` / `try_lock_until`) — только базовый `std::mutex`-подобный интерфейс.
- Изменение семантики существующих примитивов синхронизации PARAOS.
- Тестирование на физических целевых устройствах — только host-сборки.
- Глобальное переписывание CI/CD вне явно выделенных CI-задач следующей вехи.

## Context

- PARAOS — библиотека только из исходников; потребители подключают через `add_subdirectory`.
- `.clang-tidy` настроен широким набором проверок и `WarningsAsErrors: '*'`, поэтому любое предупреждение ломает сборку.
- macOS — основная машина разработки; валидация других платформ обеспечивается изоляцией изменений и сохранением существующих платформенных путей.

## Constraints

- **Стек**: C++20, CMake ≥ 3.20, Clang/GCC/MSVC, GoogleTest, clang-tidy.
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
| `jthread` API поверх `std::jthread` для PC и собственная реализация для FreeRTOS | Позволяет получить единый кроссплатформенный API с минимальными затратами на PC | ✓ Good |
| Capturing lambdas для FreeRTOS через heap-allocated invoker | `etl::delegate` не поддерживает capturing lambdas и variadic args; `std::function` требует динамического выделения | ✓ Good |
| `ThreadAttr` в конструкторе `jthread` | Необходимость задавать приоритет/стек/имя FreeRTOS-потока при создании | ✓ Good |
| Минимальная версия C++ — 20 | Использование `std::jthread`, `std::stop_token`, `std::apply`, `std::invoke` | ✓ Good |
| `paraos::mutex` API в стиле `std::mutex` | Единый кроссплатформенный API рядом с legacy `paraos::Mutex` | ✓ Good |
| Header name `paraos_mutex_std.hpp` | Avoids include-guard collision with legacy `PARAOS_MUTEX_HPP` | ✓ Good |
| FreeRTOS `paraos::mutex` поверх `xSemaphoreCreateMutex` / `xSemaphoreTake` / `xSemaphoreGive` | Минимальная реализация, совпадающая с семантикой `std::mutex` | ✓ Good |

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
*Last updated: 2026-06-22 after v1.3 milestone started*
