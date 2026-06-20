# Roadmap: PARAOS

## Milestones

- ✅ **v1.0 macOS Support** — Phases 1-4 (shipped earlier)
- ✅ **v1.1 Static Analysis Cleanup** — Phases 5-8 (shipped 2026-06-20) — see `.planning/milestones/v1.1-ROADMAP.md`
- 🚧 **v1.2 std::jthread-style Thread API** — Phases 9-12 (in progress)

## Phases

### 🚧 v1.2 std::jthread-style Thread API

- [x] **Phase 9: PC jthread implementation** — Реализовать `paraos::jthread` для Windows и Unix поверх `std::jthread` (completed 2026-06-20)
- [x] **Phase 10: FreeRTOS jthread implementation** — Реализовать `paraos::jthread` поверх FreeRTOS API с поддержкой capturing lambdas (completed 2026-06-20)
- [x] **Phase 11: Thread attributes integration** — Поддержать `ThreadAttr` (приоритет, стек, имя) в `jthread` для всех портов (completed 2026-06-20)
- [x] **Phase 12: Build, tests and static analysis** — Обновить CMake до C++20, добавить `test_jthread_basic.cpp`, прогнать все пресеты и tidy (completed 2026-06-20)

## Phase Details

### Phase 9: PC jthread implementation

**Goal**: `paraos::jthread` реализован для Windows и Unix как тонкая обёртка над `std::jthread` с единым API.
**Depends on**: Milestone v1.1 complete
**Requirements**: JTHREAD-01..06, PORT-01..03
**Success Criteria**:

  1. `port_pc/paraos_jthread.hpp` создан и содержит `paraos::jthread`, `paraos::stop_token`, `paraos::stop_source`.
  2. `port_unix/paraos_jthread.hpp` и `port_win/paraos_jthread.hpp` включают `port_pc/paraos_jthread.hpp`.
  3. `paraos::jthread` конструируется из callable и аргументов; `stop_token` передаётся последним аргументом.
  4. Поддерживаются `request_stop()`, `join()`, move-семантика и RAII-остановка в деструкторе.
  5. Публичный API не зависит от платформенных `#ifdef` в коде пользователя.

**Plans**: 1 plan

Plans:

- [x] 09-01: Реализовать `paraos::jthread` для Windows и Unix поверх `std::jthread` (JTHREAD-01..06, PORT-01..03)

### Phase 10: FreeRTOS jthread implementation

**Goal**: `paraos::jthread` реализован для FreeRTOS поверх FreeRTOS API с поддержкой capturing lambdas.
**Depends on**: Phase 9
**Requirements**: JTHREAD-01..06, PORT-04
**Success Criteria**:

  1. `port_freertos/paraos_jthread.hpp` создан и содержит собственную реализацию `paraos::jthread`.
  2. Capturing lambdas и variadic аргументы поддерживаются через heap-allocated invoker.
  3. `stop_token`, `request_stop()`, `join()` и RAII-деструктор работают корректно.
  4. Реализация не использует динамическое выделение памяти вне invoker-объекта.

**Plans**: 1 plan

Plans:

- [x] 10-01: Реализовать `paraos::jthread` поверх FreeRTOS API с поддержкой capturing lambdas (JTHREAD-01..06, PORT-04)

### Phase 11: Thread attributes integration

**Goal**: `ThreadAttr` (имя, размер стека, приоритет) поддерживается в конструкторе `jthread` для всех портов.
**Depends on**: Phase 10
**Requirements**: JTHREAD-07..10
**Success Criteria**:

  1. `paraos::jthread` предоставляет конструктор, принимающий `ThreadAttr` наряду с callable и аргументами.
  2. На FreeRTOS `ThreadAttr::priority`, `stack_depth` и `thread_name` передаются в `xTaskCreate`.
  3. На PC `ThreadAttr` корректно обрабатывается или игнорируется безопасно, если привилегий недостаточно.
  4. Старый `paraos::ThreadAttr` переиспользуется или расширяется без нарушения обратной совместимости.

**Plans**: 1 plan

Plans:

- [x] 11-01: Поддержать `ThreadAttr` (приоритет, стек, имя) в `jthread` для всех портов (JTHREAD-07..10)

### Phase 12: Build, tests and static analysis

**Goal**: Сборка, тесты и статический анализ проходят для нового `jthread` API.
**Depends on**: Phase 11
**Requirements**: BUILD-01..02, TEST-01..05
**Success Criteria**:

  1. Минимальная версия C++ в корневом `CMakeLists.txt` повышена до C++20.
  2. `paraos_jthread.hpp` доступен для включения из всех портов через CMake.
  3. `port_tests/test_jthread_basic.cpp` создан, зарегистрирован в CTest и проходит на `pc_debug_clang`, `pc_debug_gcc`, `freertos_debug_clang`, `freertos_debug_gcc`.
  4. `*_clang_tidy` пресеты собираются без новых предупреждений.
  5. Все не-tidy пресеты продолжают собираться без регрессий.

**Plans**: 1 plan

Plans:

- [x] 12-01: Обновить CMake до C++20, добавить `test_jthread_basic.cpp`, прогнать все пресеты и tidy (BUILD-01..02, TEST-01..05)

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
| ----- | --------- | -------------- | ------ | --------- |
| 9. PC jthread implementation | v1.2 | 1/1 | Complete | 2026-06-20 |
| 10. FreeRTOS jthread implementation | v1.2 | 1/1 | Complete | 2026-06-20 |
| 11. Thread attributes integration | v1.2 | 1/1 | Complete | 2026-06-20 |
| 12. Build, tests and static analysis | v1.2 | 1/1 | Complete | 2026-06-20 |
