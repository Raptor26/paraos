# Roadmap: PARAOS — Milestone v1.10

**Milestone:** v1.10 — Modernize Unix timer with paraos primitives  
**Phases:** 40–44 (continuous numbering from v1.9 Phase 39)  
**Defined:** 2026-06-25  
**Core Value:** Кроссплатформенная переносимость PARAOS сохраняется: код, работающий на Linux/Windows/FreeRTOS, продолжает работать, а новая macOS-разработка ведётся на равных с остальными платформами, включая статический анализ clang-tidy.

## Milestone Goal

Заменить POSIX timer API (`timer_create`/`timer_settime`/`timer_delete`) и pthread API (`pthread_mutex_*`, `pthread_cond_*`, `pthread_create`/`pthread_join`) в `port_unix/paraos_timer.hpp` на единую кроссплатформенную реализацию поверх `paraos::jthread`, `paraos::mutex` и `paraos::*_semaphore`, сохранив публичный API `paraos::timer` и поведение на всех платформах.

---

## Phase 40: Design and test scaffold

**Goal:** Подготовить скелет новой реализации и тестовую инфраструктуру до начала разработки цикла таймера.

**Requirements covered:** TMR-04, TEST-01

**Deliverables:**
- Обновлённый `port_unix/paraos_timer.hpp`: новые includes (`paraos_jthread.hpp`, `paraos_mutex_std.hpp`, `paraos_semaphore_std.hpp`, `paraos_sleep.hpp`), очищенные от POSIX/pthread private-члены, placeholder-методы.
- Новый `port_tests/test_timer.cpp` с минимальным skeleton: класс-наследник `paraos::timer`, пустой `run()`, `main()`.
- Обновлённый `port_tests/CMakeLists.txt`: standalone-цель `test_timer` зарегистрирована с `cxx_std_20`, `TIMEOUT 20` и прикреплена к `CXX_CLANG_TIDY`.

**Success Criteria:**
1. `port_unix/paraos_timer.hpp` компилируется без `timer_create`, `timer_settime`, `timer_delete`, `pthread_mutex_*`, `pthread_cond_*`, `pthread_create`, `pthread_join` на пути компиляции Linux/macOS.
2. `port_tests/test_timer.cpp` компилируется в пресетах `pc_debug_clang` и `pc_debug_gcc`.
3. Новая цель `test_timer` видна в `ctest -N` в PC-сборках.
4. Сигнатура публичного API `paraos::timer` (конструктор, `start()`, `stop()`, `reset()`, `change_period()`, виртуальный `run()`) идентична существующей.

---

## Phase 41: Core jthread-based loop

**Goal:** Реализовать рабочий поток таймера поверх PARAOS-примитивов, поддерживающий периодический и one-shot режимы без дрейфа.

**Requirements covered:** TMR-01, TMR-02, TMR-03, TMR-05, TMR-06, TMR-11, TMR-12

**Deliverables:**
- `std::optional<paraos::jthread>` worker_, создаваемый в `start()` и уничтожаемый в `stop()`.
- Цикл рабочего потока на `std::chrono::steady_clock` с дедлайном, `binary_semaphore::try_acquire_for()` для ожидания оставшегося времени.
- Вызов виртуального `run()` без удержания `mutex_`.
- Ветвление `is_auto_reload_` для периодического/one-shot режимов.
- Защита mutable-состояния (`period_ms_`, флаги) через `paraos::mutex`.

**Success Criteria:**
1. Таймер с `is_auto_reload=true` вызывает `run()` несколько раз с интервалом, отклонение которого от заданного `period_ms` не превышает допустимого джиттера ОС (измеряется в `test_timer.cpp`).
2. Таймер с `is_auto_reload=false` вызывает `run()` ровно один раз после `period_ms`.
3. Сборка `pc_debug_clang` и `pc_debug_gcc` проходит без ошибок; таймер создаёт поток только после `start()`.
4. Отсутствуют вызовы POSIX timer API и pthread API в `port_unix/paraos_timer.hpp` на обеих платформах; ветвления `__linux__` / `__APPLE__` удалены.
5. Внутри цикла `mutex_` не удерживается при вызове `run()`.

---

## Phase 42: Start/stop/reset/change_period synchronization

**Goal:** Обеспечить корректную синхронизацию публичных методов с рабочим потоком, безопасный stop изнутри `run()` и безопасный деструктор.

**Requirements covered:** TMR-07, TMR-08, TMR-09, TMR-10

**Deliverables:**
- `start()`: повторный запуск пересчитывает дедлайн от текущего момента; не создаёт второй поток, если таймер уже запущен.
- `stop()`: устанавливает флаг остановки, будит семафор, join-ит worker; защищён от self-deadlock при вызове из `run()`.
- `reset()`: пересчитывает дедлайн и будит спящий рабочий поток.
- `change_period()`: атомарно обновляет `period_ms_` и будит рабочий поток для немедленного применения.
- Деструктор: вызывает `stop()` до разрушения объекта.

**Success Criteria:**
1. `start()` на уже запущенном таймере сбрасывает отсчёт; следующий `run()` происходит через полный `period_ms` от момента вызова.
2. `change_period()` применяет новый период до следующего срабатывания, а не после текущего цикла.
3. `stop()`, вызванный изнутри `run()`, не вызывает self-deadlock и корректно завершает рабочий поток.
4. Деструктор останавливает и join-ит worker даже при активном таймере.
5. Нет потерянных или повторяющихся wakeup: repeated `release()` на binary_semaphore не приводит к исключению или пропуску цикла.

---

## Phase 43: Standalone test and documentation

**Goal:** Расширить standalone-тест до покрытия всех режимов и задокументировать требование планировщика.

**Requirements covered:** TEST-02, TEST-03, TEST-04, TEST-05, TEST-06

**Deliverables:**
- `port_tests/test_timer.cpp` с тестами: периодический режим, one-shot, `change_period()`, `reset()`, `stop()` изнутри `run()`, безопасность деструктора.
- Явный вызов `paraos::jthread::start_scheduler()` перед ожиданием callbacks и `paraos::jthread::end_scheduler()` в конце.
- Doxygen-комментарии в `port_unix/paraos_timer.hpp`, объясняющие гейтинг планировщика и поведение `is_auto_reload`.

**Success Criteria:**
1. `test_timer` проходит в пресетах `pc_debug_clang` и `pc_debug_gcc`.
2. Тест покрывает периодический режим (≥3 срабатывания) и one-shot (ровно 1 срабатывание).
3. Тест проверяет `change_period()` и `reset()` через измерение фактических интервалов.
4. Тест проверяет `stop()` и деструктор (таймер уничтожается без зависания).
5. В `main()` явно вызываются `start_scheduler()` / `end_scheduler()`.

---

## Phase 44: Static analysis and cross-platform regression

**Goal:** Подтвердить отсутствие регрессий в PC/FreeRTOS/Windows портах и чистоту clang-tidy.

**Requirements covered:** BLD-01, BLD-02, BLD-03, BLD-04

**Deliverables:**
- Прохождение `ctest` для `pc_debug_clang`, `pc_debug_gcc`.
- Прохождение `*_clang_tidy` пресетов без новых предупреждений от кода таймера и `test_timer.cpp`.
- Успешная конфигурация/сборка FreeRTOS-пресетов (`freertos_debug_clang`, `freertos_debug_gcc`).
- Подтверждение, что `port_win/paraos_timer.hpp` и `port_freertos/paraos_timer.hpp` не изменены.

**Success Criteria:**
1. `pc_debug_clang` проходит `ctest --output-on-failure --stop-on-failure` без регрессий (базовое число тестов 58/58 + новый `test_timer`).
2. `pc_debug_gcc` проходит `ctest` без регрессий.
3. `pc_debug_gcc_clang_tidy` (или доступный `*_clang_tidy` пресет) собирается без новых предупреждений от `port_unix/paraos_timer.hpp` и `port_tests/test_timer.cpp`.
4. `freertos_debug_clang` и `freertos_debug_gcc` успешно компилируются.
5. Git diff не содержит изменений в `port_win/paraos_timer.hpp` и `port_freertos/paraos_timer.hpp`.

---

## Coverage Summary

| Phase | Name | Requirements Covered | Count |
|-------|------|----------------------|-------|
| 40 | Design and test scaffold | TMR-04, TEST-01 | 2 |
| 41 | Core jthread-based loop | TMR-01, TMR-02, TMR-03, TMR-05, TMR-06, TMR-11, TMR-12 | 7 |
| 42 | Start/stop/reset/change_period synchronization | TMR-07, TMR-08, TMR-09, TMR-10 | 4 |
| 43 | Standalone test and documentation | TEST-02, TEST-03, TEST-04, TEST-05, TEST-06 | 5 |
| 44 | Static analysis and cross-platform regression | BLD-01, BLD-02, BLD-03, BLD-04 | 4 |

**Total v1 requirements mapped:** 22 / 22 (100%)  
**Unmapped:** 0

---
*Roadmap created: 2026-06-25*
