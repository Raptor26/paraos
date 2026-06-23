# Requirements: PARAOS v1.7

**Defined:** 2026-06-23
**Core Value:** Кроссплатформенная переносимость PARAOS сохраняется: код, работающий на Linux/Windows/FreeRTOS, продолжает работать, а новая macOS-разработка ведётся на равных с остальными платформами, включая статический анализ clang-tidy.

## v1.7 Requirements

### Test Migration

- [ ] **MIG-01**: `test_thread_only_stack.cpp` переведён с `paraos::Thread` на `paraos::jthread`.
- [ ] **MIG-02**: `test_thread_only_stack_with_multiple_threads.cpp` переведён с `paraos::Thread` на `paraos::jthread`.
- [ ] **MIG-03**: `test_thread_only_static.cpp` переведён с `paraos::Thread` на `paraos::jthread`.
- [ ] **MIG-04**: `test_thread_only_global.cpp` переведён с `paraos::Thread` на `paraos::jthread`.

### Cross-Platform Lifecycle

- [ ] **LIFE-01**: Во всех четырёх тестах используется единый паттерн запуска/завершения: `paraos::jthread::start_scheduler()`, `IdleHook()` → `WaitForSchedulerEnded()` → `CheckIfTestSuccessfullyComplete()` → `paraos::jthread::end_scheduler()`.
- [ ] **LIFE-02**: Для FreeRTOS-пути устанавливается `paraos::freertos_idle_fnc_ptr = IdleHook;` под `#if PARAOS_LIKE_FREERTOS`.
- [ ] **LIFE-03**: Удалены платформенные ветви `std::_Exit()`, `paraos::Thread::Exit()`, `paraos::Thread::DeleteAll()` и ручной `new`/`delete` потоков там, где это возможно.

### Idiomatic C++

- [ ] **IDIO-01**: Создание и удаление потоков выполнено идиоматично для C++: RAII-контейнеры (`std::vector<paraos::jthread>`), `std::jthread`-стиль лямбд с `paraos::stop_token`, автоматическое объединение/остановка через деструкторы.

### Build & Static Analysis

- [ ] **BUILD-01**: `port_tests/CMakeLists.txt` переключил четыре цели `test_thread_only_*` на `cxx_std_20`.
- [ ] **BUILD-02**: Четыре цели `test_thread_only_*` включены в `CXX_CLANG_TIDY` при `CLANG_TIDY_ENABLE`.
- [ ] **BUILD-03**: Все доступные CMake-пресеты на macOS конфигурируются и собираются без новых предупреждений clang-tidy.

### Test Execution

- [ ] **TEST-01**: Каждый из четырёх тестов в CTest зарегистрирован с таймаутом 20 секунд.
- [ ] **TEST-02**: `ctest` для PC-пресетов (`pc_debug_clang`, `pc_debug_gcc`) на macOS проходит полностью в пределах таймаута.
- [ ] **TEST-03**: FreeRTOS-пресеты (`freertos_debug_clang`, `freertos_debug_gcc`) на macOS не только собираются, но и успешно выполняются и завершаются в пределах таймаута 20 секунд.
- [ ] **TEST-04**: Код остаётся кроссплатформенным для Windows и Linux; прямой runtime-запуск на этих платформах не проверяется на текущем macOS-хосте, но регрессии в платформенных путях отсутствуют.

## v2 Requirements

### Future Test Modernization

- **FUT-01**: Миграция оставшихся standalone-тестов и примеров `port_tests/` на `paraos::jthread` (например, `example_thread_check_timeout`).
- **FUT-02**: Полный переход `extra/` и production-кода на `paraos::jthread` / `paraos::mutex`.

## Out of Scope

| Feature | Reason |
|---------|--------|
| Изменение реализации `paraos::Thread` | Веха касается только тестов; legacy API сохраняется. |
| Миграция GoogleTest-тестов (`test_paraos_core`) | Цель — standalone-тесты `test_thread_only_*`. |
| Изменение публичного API PARAOS | Новые возможности `paraos::jthread` уже доступны; задача — использовать их в тестах. |
| Windows/Linux runtime verification на физических машинах | Текущий хост — macOS; проверка через сохранение кроссплатформенных путей и изоляцию изменений. |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| MIG-01 | Phase 28 | Pending |
| MIG-02 | Phase 28 | Pending |
| MIG-03 | Phase 28 | Pending |
| MIG-04 | Phase 28 | Pending |
| LIFE-01 | Phase 28 | Pending |
| LIFE-02 | Phase 28 | Pending |
| LIFE-03 | Phase 28 | Pending |
| IDIO-01 | Phase 28 | Pending |
| BUILD-01 | Phase 29 | Pending |
| BUILD-02 | Phase 29 | Pending |
| BUILD-03 | Phase 29 | Pending |
| TEST-01 | Phase 30 | Pending |
| TEST-02 | Phase 30 | Pending |
| TEST-03 | Phase 30 | Pending |
| TEST-04 | Phase 30 | Pending |

**Coverage:**
- v1.7 requirements: 15 total
- Mapped to phases: 15
- Unmapped: 0 ✓

---
*Requirements defined: 2026-06-23*
*Last updated: 2026-06-23 after initial definition*
