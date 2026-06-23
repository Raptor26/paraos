# Requirements: PARAOS v1.8

## Milestone

**v1.8 Migrate `extra/` libraries to `paraos::jthread`**

**Goal:** Перевести внутреннюю реализацию библиотек `extra/` с legacy `paraos::Thread` на `paraos::jthread`, сохранив публичный API и совместимость с FreeRTOS/PC.

## Categories

### Threading migration

- [ ] **MIG-01**: Внутренний поток `extra/paraos_oneshot_executor.hpp` использует `paraos::jthread` вместо `paraos::Thread`.
- [ ] **MIG-02**: Внутренний поток `extra/paraos_thread_sequence.hpp` использует `paraos::jthread` вместо `paraos::Thread`.
- [ ] **MIG-03**: Внутренний поток `extra/paraos_thread_cooperative_scheduling.hpp` использует `paraos::jthread` вместо `paraos::Thread`.
- [ ] **MIG-04**: Standalone-тесты `extra/tests/test_*_thread.cpp` мигрированы на `paraos::jthread`, `start_scheduler()` и `end_scheduler()`.

### Build & static analysis

- [ ] **BUILD-01**: `extra/tests/CMakeLists.txt` требует `cxx_std_20` для всех целей.
- [ ] **BUILD-02**: Для standalone-целей `extra/tests/` (`test_paraos_thread_sequence`, `test_paraos_cooperative_scheduling_thread`, `test_paraos_oneshot_executor`) добавлен `CXX_CLANG_TIDY` в режимах `CLANG_TIDY_ENABLE`.
- [ ] **BUILD-03**: Все изменённые заголовки и тесты проходят `*_clang_tidy` пресеты без новых предупреждений.

### Verification

- [ ] **TEST-01**: PC-пресеты (`pc_debug_clang`, `pc_debug_gcc`, `pc_debug_gcc_clang_tidy`) проходят `ctest` без регрессий.
- [ ] **TEST-02**: FreeRTOS-пресеты (`freertos_debug_clang`, `freertos_debug_gcc`) компилируются и проходят доступные runtime-тесты в пределах ограничений POSIX-порта macOS.

## Future Requirements

- Перевод production-кода вне `extra/` на `paraos::jthread` / `paraos::mutex`.
- Депрекация и постепенное удаление legacy `paraos::Thread` после полной миграции всех потребителей.

## Out of Scope

- Изменение публичных сигнатур классов `extra/` (`OneShotExecutor`, `ThreadSequence`, `CooperativeScheduling`, `StatusLed`).
- Замена `paraos::Mutex` на `paraos::mutex` внутри `extra/` — остаётся legacy API.
- Миграция контейнеров и `port_tests/` — уже выполнено в предыдущих вехах.
- Глобальное переписывание CI/CD.

## Traceability

| Req-ID | Phase | Status |
|--------|-------|--------|
| MIG-01 | Phase 31 | Open |
| MIG-02 | Phase 32 | Open |
| MIG-03 | Phase 33 | Open |
| MIG-04 | Phase 34 | Open |
| BUILD-01 | Phase 34 | Open |
| BUILD-02 | Phase 34 | Open |
| BUILD-03 | Phase 35 | Open |
| TEST-01 | Phase 35 | Open |
| TEST-02 | Phase 35 | Open |
