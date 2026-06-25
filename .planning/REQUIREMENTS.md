# Requirements: PARAOS — Milestone v1.10

**Defined:** 2026-06-25
**Core Value:** Кроссплатформенная переносимость PARAOS сохраняется: код, работающий на Linux/Windows/FreeRTOS, продолжает работать, а новая macOS-разработка ведётся на равных с остальными платформами, включая статический анализ clang-tidy.

## v1 Requirements

### Timer Core (TMR)

- [ ] **TMR-01**: `port_unix/paraos_timer.hpp` не использует POSIX timer API (`timer_create`, `timer_settime`, `timer_delete`) на Linux.
- [ ] **TMR-02**: `port_unix/paraos_timer.hpp` не использует pthread API (`pthread_mutex_*`, `pthread_cond_*`, `pthread_create`, `pthread_join`) на macOS.
- [ ] **TMR-03**: Реализация `paraos::timer` для Linux и macOS объединена в единый кодовый путь без ветвления `#ifdef __linux__` / `__APPLE__`.
- [ ] **TMR-04**: Публичный API `paraos::timer` сохранён: конструктор `(period_ms, start_immediately, is_auto_reload, name)`; методы `start()`, `stop()`, `reset()`, `change_period()`; виртуальный `run()`.
- [ ] **TMR-05**: Периодический режим (`is_auto_reload == true`) вызывает `run()` с заданным периодом без накопления дрейфа.
- [ ] **TMR-06**: One-shot режим (`is_auto_reload == false`) вызывает `run()` один раз после `period_ms`.
- [ ] **TMR-07**: `start()` на уже запущенном таймере пересчитывает время следующего срабатывания от текущего момента.
- [ ] **TMR-08**: `change_period()` немедленно применяет новый период и будит спящий рабочий поток.
- [ ] **TMR-09**: `stop()` корректно останавливает рабочий поток, join-ит его и не приводит к self-deadlock при вызове изнутри `run()`.
- [ ] **TMR-10**: Деструктор останавливает и join-ит рабочий поток до разрушения объекта.
- [ ] **TMR-11**: Внутреннее состояние защищено `paraos::mutex`; для ожидания периода используется `paraos::binary_semaphore::try_acquire_for()` с возможностью раннего пробуждения.
- [ ] **TMR-12**: Для рабочего потока используется `std::optional<paraos::jthread>` с отложенным созданием в `start()`.

### Tests (TEST)

- [ ] **TEST-01**: Добавлен standalone-тест `port_tests/test_timer.cpp`, демонстрирующий максимально простой способ использования `paraos::timer` в приложении пользователя.
- [ ] **TEST-02**: Тест покрывает периодический режим.
- [ ] **TEST-03**: Тест покрывает one-shot режим.
- [ ] **TEST-04**: Тест покрывает `change_period()` и `reset()`.
- [ ] **TEST-05**: Тест покрывает `stop()` и безопасность деструктора.
- [ ] **TEST-06**: Тест явно вызывает `paraos::jthread::start_scheduler()` / `end_scheduler()` для демонстрации требования планировщика.

### Build & Regression (BLD)

- [ ] **BLD-01**: PC-пресеты (`pc_debug_clang`, `pc_debug_gcc`) проходят `ctest` без регрессий.
- [ ] **BLD-02**: `*_clang_tidy` пресеты собираются без новых предупреждений.
- [ ] **BLD-03**: FreeRTOS-пресеты (`freertos_debug_clang`, `freertos_debug_gcc`) успешно компилируются.
- [ ] **BLD-04**: `port_win/paraos_timer.hpp` и `port_freertos/paraos_timer.hpp` не изменены.

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Timer Enhancements

- **TMRV2-01**: Перенос имени таймера в `paraos::thread_attr` для отладочной видимости.
- **TMRV2-02**: Общий пул потоков для нескольких программных таймеров.
- **TMRV2-03**: Публичные API pause/resume или смещения первого запуска.

## Out of Scope

| Feature | Reason |
|---------|--------|
| Изменение публичного API `paraos::timer` | Цель вехи — замена реализации, а не эволюция контракта. |
| Изменение `port_win/paraos_timer.hpp` | Windows-реализация не использует pthread/POSIX timer API. |
| Изменение `port_freertos/paraos_timer.hpp` | FreeRTOS-реализация не использует pthread/POSIX timer API. |
| Добавление ISR-контекста для Unix | Unix-порт игнорирует `is_isr`; параметр сохранён для совместимости. |
| Runtime-запуск FreeRTOS-тестов на macOS | Environment limitation, не решается в этой вехе. |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| TMR-01 | Phase 2 | Pending |
| TMR-02 | Phase 2 | Pending |
| TMR-03 | Phase 2 | Pending |
| TMR-04 | Phase 1 | Pending |
| TMR-05 | Phase 2 | Pending |
| TMR-06 | Phase 2 | Pending |
| TMR-07 | Phase 3 | Pending |
| TMR-08 | Phase 3 | Pending |
| TMR-09 | Phase 3 | Pending |
| TMR-10 | Phase 3 | Pending |
| TMR-11 | Phase 2 | Pending |
| TMR-12 | Phase 2 | Pending |
| TEST-01 | Phase 1 | Pending |
| TEST-02 | Phase 4 | Pending |
| TEST-03 | Phase 4 | Pending |
| TEST-04 | Phase 4 | Pending |
| TEST-05 | Phase 4 | Pending |
| TEST-06 | Phase 4 | Pending |
| BLD-01 | Phase 5 | Pending |
| BLD-02 | Phase 5 | Pending |
| BLD-03 | Phase 5 | Pending |
| BLD-04 | Phase 5 | Pending |

**Coverage:**
- v1 requirements: 22 total
- Mapped to phases: 22
- Unmapped: 0 ✓

---
*Requirements defined: 2026-06-25*
*Last updated: 2026-06-25 after initial definition*
