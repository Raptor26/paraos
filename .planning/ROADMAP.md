# Roadmap: PARAOS

## Milestones

- ✅ **v1.0 macOS Support** — Phases 1-4 (shipped earlier)
- ✅ **v1.1 Static Analysis Cleanup** — Phases 5-8 (shipped 2026-06-20) — see `.planning/milestones/v1.1-ROADMAP.md`
- ✅ **v1.2 std::jthread-style Thread API** — Phases 9-12 (shipped 2026-06-20) — see `.planning/milestones/v1.2-ROADMAP.md`
- ✅ **v1.3 std::mutex-style Mutex API** — Phases 13-15 (shipped 2026-06-22) — see `.planning/milestones/v1.3-ROADMAP.md`
- ✅ **v1.4 std::semaphore-style Semaphore API** — Phases 16-18 (shipped 2026-06-22) — see `.planning/milestones/v1.4-ROADMAP.md`
- ✅ **v1.5 Modernize container tests on std-like primitives** — Phases 19-23 (shipped 2026-06-22) — see `.planning/milestones/v1.5-ROADMAP.md`
- ✅ **v1.6 paraos::jthread scheduler control** — Phases 24-27 (shipped 2026-06-22) — see `.planning/milestones/v1.6-ROADMAP.md`
- 🚧 **v1.7 Migrate `test_thread_only_*` to `paraos::jthread`** — Phases 28-30 (in planning)

## Phases

### 🚧 v1.7 Migrate `test_thread_only_*` to `paraos::jthread` (In Planning)

**Milestone Goal:** Перевести четыре standalone-теста `port_tests/test_thread_only_*.cpp` с legacy `paraos::Thread` на `paraos::jthread`, сохранив функциональность, обеспечив идиоматичное C++ управление жизненным циклом потоков и прохождение сборки и тестов.

#### Phase 28: Migrate `test_thread_only_*` sources to `paraos::jthread`
**Goal**: Четыре standalone-теста `port_tests/test_thread_only_*.cpp` переписаны с использованием `paraos::jthread` и единого кроссплатформенного паттерна завершения.
**Depends on**: Phase 27
**Requirements**: MIG-01..MIG-04, LIFE-01..LIFE-03, IDIO-01
**Success Criteria** (what must be TRUE):
  1. `test_thread_only_stack.cpp`, `test_thread_only_stack_with_multiple_threads.cpp`, `test_thread_only_static.cpp` и `test_thread_only_global.cpp` используют `paraos::jthread` и `paraos::stop_token`.
  2. Удалены `paraos::Thread`, `Thread::StartScheduler()`, `Thread::Exit()`, `Thread::DeleteAll()` и `std::_Exit()`.
  3. Потоки управляются RAII-контейнерами (`std::vector<paraos::jthread>`); ручное `new`/`delete` минимизировано.
  4. Единый паттерн завершения работает для PC и FreeRTOS (через `IdleHook` + `paraos::freertos_idle_fnc_ptr`).
  5. Код компилируется без новых предупреждений clang-tidy.
**Plans**: 0/1 complete

Plans:
- [x] 28-01: Переписать четыре `test_thread_only_*.cpp` на `paraos::jthread` с единым паттерном завершения

#### Phase 28: Migrate `test_thread_only_*` sources to `paraos::jthread` | v1.7 | 1/1 | Complete | 2026-06-23

#### Phase 29: Update `port_tests/CMakeLists.txt` for new tests
**Goal**: Сборочная система приведена в соответствие с мигрированными тестами: C++20, clang-tidy, таймаут 20 секунд.
**Depends on**: Phase 28
**Requirements**: BUILD-01..BUILD-04
**Success Criteria** (what must be TRUE):
  1. Четыре цели `test_thread_only_*` переключены на `cxx_std_20`.
  2. Четыре цели включены в `CXX_CLANG_TIDY` при `CLANG_TIDY_ENABLE`.
  3. Каждый тест зарегистрирован в CTest с таймаутом 20 секунд.
  4. Все CMake-пресеты на macOS конфигурируются и собираются без новых предупреждений.
**Plans**: 0/1 complete

Plans:
- [x] 29-01: Обновить `port_tests/CMakeLists.txt`: `cxx_std_20`, `CXX_CLANG_TIDY`, `TIMEOUT 20`

#### Phase 29: Update `port_tests/CMakeLists.txt` for new tests | v1.7 | 1/1 | Complete | 2026-06-23

#### Phase 30: Runtime verification on macOS
**Goal**: Все тесты проходят на macOS с таймаутом 20 секунд, включая FreeRTOS-пресеты.
**Depends on**: Phase 29
**Requirements**: TEST-01..TEST-03
**Success Criteria** (what must be TRUE):
  1. `ctest` для PC-пресетов (`pc_debug_clang`, `pc_debug_gcc`) на macOS проходит полностью в пределах таймаута 20 секунд.
  2. FreeRTOS-пресеты (`freertos_debug_clang`, `freertos_debug_gcc`) на macOS не только собираются, но и успешно выполняются и завершаются в пределах 20 секунд.
  3. Код остаётся кроссплатформенным для Windows и Linux; регрессии в платформенных путях отсутствуют.
**Plans**: 0/1 complete

Plans:
- [x] 30-01: Запустить `ctest --timeout 20` для PC-пресетов и FreeRTOS-пресетов на macOS

#### Phase 30: Runtime verification on macOS | v1.7 | 1/1 | Complete | 2026-06-23

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
| ----- | --------- | -------------- | ------ | --------- |
| 1-4. macOS Support | v1.0 | 4/4 | Complete | earlier |
| 5. Reproduce & Classify clang-tidy warnings | v1.1 | 1/1 | Complete | 2026-06-20 |
| 6. Fix Core, Headers & port_unix | v1.1 | 1/1 | Complete | 2026-06-20 |
| 7. Fix Tests, Examples & Document Suppressions | v1.1 | 1/1 | Complete | 2026-06-20 |
| 8. Regression Guard | v1.1 | 1/1 | Complete | 2026-06-20 |
| 9. PC jthread implementation | v1.2 | 1/1 | Complete | 2026-06-20 |
| 10. FreeRTOS jthread implementation | v1.2 | 1/1 | Complete | 2026-06-20 |
| 11. Thread attributes integration | v1.2 | 1/1 | Complete | 2026-06-20 |
| 12. Build, tests and static analysis | v1.2 | 1/1 | Complete | 2026-06-20 |
| 13. PC mutex implementation | v1.3 | 1/1 | Complete | 2026-06-22 |
| 14. FreeRTOS mutex implementation | v1.3 | 1/1 | Complete | 2026-06-22 |
| 15. Build, tests and static analysis | v1.3 | 1/1 | Complete | 2026-06-22 |
| 16. PC counting_semaphore implementation | v1.4 | 1/1 | Complete | 2026-06-22 |
| 17. FreeRTOS counting_semaphore implementation | v1.4 | 1/1 | Complete | 2026-06-22 |
| 18. Build, tests and static analysis | v1.4 | 1/1 | Complete | 2026-06-22 |
| 19. Inventory & gap analysis | v1.5 | 1/1 | Complete | 2026-06-20 |
| 20. Migrate thread primitives in container tests | v1.5 | 1/1 | Complete | 2026-06-22 |
| 21. Migrate synchronization primitives in container tests | v1.5 | 1/1 | Complete | 2026-06-22 |
| 22. FreeRTOS std-like primitives hardening | v1.5 | 1/1 | Complete | 2026-06-22 |
| 23. Build, tests and static analysis | v1.5 | 1/1 | Complete | 2026-06-22 |
| 24. FreeRTOS scheduler API | v1.6 | 2/2 | Complete | 2026-06-22 |
| 25. PC scheduler state and gating | v1.6 | 3/3 | Complete | 2026-06-22 |
| 26. Test unification | v1.6 | 7/7 | Complete | 2026-06-22 |
| 27. Build and static analysis verification | v1.6 | 6/6 | Complete | 2026-06-22 |
| 28. Migrate `test_thread_only_*` sources to `paraos::jthread` | v1.7 | 1/1 | Complete | 2026-06-23 |
| 29. Update `port_tests/CMakeLists.txt` for new tests | v1.7 | 1/1 | Complete | 2026-06-23 |
| 30. Runtime verification on macOS | v1.7 | 1/1 | Complete | 2026-06-23 |
