# Project Research Summary: Modernize container tests on std-like primitives

**Project:** PARAOS
**Domain:** C++ OSAL / embedded / cross-platform primitives
**Researched:** 2026-06-22
**Confidence:** HIGH

## Executive Summary

PARAOS уже предоставляет современные C++20-обёртки `paraos::jthread`, `paraos::mutex`, `paraos::counting_semaphore` / `paraos::binary_semaphore`, реализованные поверх `std::` для PC и поверх FreeRTOS API для `port_freertos`. В то же время multithread-тесты в `containers/tests/` продолжают использовать legacy-примитивы `paraos::Thread`, `paraos::Mutex` и `paraos::SemaphoreBinary`. Это создаёт технический долг: новые std-like примитивы не проверяются в реальных многопоточных сценариях с контейнерами, а тесты остаются привязаны к специфичной для legacy-API модели (делегаты, `StartScheduler`, `Finished`, `DeleteAll`).

Рекомендуемый подход — постепенная миграция: сначала инвентаризация и анализ разрывов API, затем перевод потоков на `paraos::jthread`, затем замена мьютексов/семафоров там, где это семантически уместно, и, наконец, минимальные доработки `port_freertos` для единообразия. Ключевые риски — deadlock при неправильном завершении FreeRTOS-задач, различия в приоритетах/именах потоков и ограничения POSIX-порта FreeRTOS на macOS, из-за которых runtime-запуск остаётся недоступен.

## Key Findings

### Recommended Stack

- **C++20** — минимальная версия уже установлена; используется для `std::jthread`, `std::counting_semaphore`, концептов и `requires`.
- **PC-порты (`port_pc/` → `port_unix/`, `port_win/`)** — тонкие обёртки над `std::jthread`, `std::mutex`, `std::counting_semaphore`; не требуют изменений.
- **FreeRTOS-порт (`port_freertos/`)** — собственные реализации `paraos::jthread` (heap-allocated invoker + `xTaskCreate`), `paraos::mutex` (`xSemaphoreCreateMutex`) и `paraos::counting_semaphore` (`xSemaphoreCreateCounting`).
- **GoogleTest / CTest** — сохраняются; стресс-тесты помечены меткой `stress`.
- **clang-tidy** — `.clang-tidy` с `WarningsAsErrors: '*'`, любое новое предупреждение ломает сборку.

### Expected Features

**Table stakes:**
- Единый пользовательский API для `paraos::jthread`, `paraos::mutex`, `paraos::*_semaphore` на PC и FreeRTOS.
- Совместимость `paraos::mutex` с `std::lock_guard` и `std::unique_lock`.
- Совместимость `paraos::counting_semaphore` с `std::chrono` (`try_acquire_for`, `try_acquire_until`).
- Сохранение регрессионного покрытия: PC `ctest` и `*_clang_tidy` пресеты.

**Differentiators:**
- Использование современных RAII-обёрток в тестах повышает читаемость и снижает риск утечек.
- Capturing lambdas в `paraos::jthread` на FreeRTOS через heap-allocated invoker.

**Defer (v2+):**
- Переписывание тестов вне `containers/tests/`.
- Замена внутренней реализации контейнеров (`paraos_queue_blocking.hpp` и др.).
- Удаление или депрекация legacy-примитивов.

### Architecture Approach

Каждый multithread-тест представляет собой набор producer/consumer задач, синхронизируемых через shared контейнер и атомарные счётчики. Legacy-реализация использует:

- `paraos::Thread` + `thread_delegate_type::create` для запуска метода класса в отдельном потоке;
- `paraos::ThreadAttr` для имени, приоритета и размера стека;
- `paraos::Thread::StartScheduler()` / `DeleteAll()` / `Exit()` для управления жизненным циклом;
- `paraos::Mutex` / `MutexGuard` и `paraos::SemaphoreBinary` / `SemaphoreCounting` для синхронизации;
- `paraos::CriticalSection` для защиты вывода / атомарных проверок.

Миграция на std-like примитивы заменяет:

- `paraos::Thread` + делегат → `paraos::jthread(ThreadAttr, lambda)`;
- `Finished()` → natural scope exit / `request_stop()` + `join()`;
- `StartScheduler()` / `DeleteAll()` / `Exit()` → PC: `join()` + `paraos::Thread::Exit()`; FreeRTOS: `std::_Exit()` изнутри scheduler;
- `paraos::Mutex` → `paraos::mutex` + `std::lock_guard`;
- `paraos::SemaphoreBinary` → `paraos::binary_semaphore`.

### Critical Pitfalls

1. **Deadlock при завершении FreeRTOS-задач.** `paraos::jthread::join()` ждёт `join_sem.Take()`; если задача завершилась до вызова `join()`, семафор уже выдан — `Take()` пройдёт. Но если `request_stop()` вызван после удаления задачи, `context_->handle` может стать висячим указателем.
2. **Имена потоков в `paraos::jthread`.** `GiveName()` отсутствует в std-like API; тесты активно используют `thread_.GiveName()` для отладочного вывода. Нужен способ получать имя потока или заменить вывод.
3. **Priority / stack в `paraos::jthread`.** `ThreadAttr` поддержан в конструкторе, но `SetPriority` / `GetPriority` / изменение стека недоступны.
4. **`std::_Exit()` на FreeRTOS.** POSIX-порт FreeRTOS на macOS не возвращает из `vTaskStartScheduler()`; финализация тестов должна происходить изнутри scheduler.
5. **`paraos::counting_semaphore::release(N)` на FreeRTOS.** Текущая реализация приостанавливает scheduler; нужно убедиться, что это корректно в многопоточных тестах.
6. **clang-tidy.** Любая новая суппрессия должна быть явно обоснована.

## Implications for Roadmap

### Phase 19: Inventory & gap analysis
**Rationale:** Нельзя мигрировать без полного понимания текущих зависимостей и разрывов API.
**Delivers:** Таблица использования legacy-примитивов по тестам, список API-гэпов, перечень доработок `port_freertos`.
**Addresses:** Анализ (ANL-01, ANL-02, ANL-03).
**Avoids:** Неполная миграция и неожиданные deadlock на FreeRTOS.

### Phase 20: Migrate thread primitives in container tests
**Rationale:** Потоковая модель — самая большая разница между legacy и std-like API.
**Delivers:** Все multithread-тесты `containers/tests/` используют `paraos::jthread`.
**Addresses:** THR-01..THR-04.
**Avoids:** Pitfall 1, Pitfall 4.

### Phase 21: Migrate synchronization primitives in container tests
**Rationale:** После замены потоков можно заменить `Mutex`/`Semaphore` на std-like обёртки.
**Delivers:** `paraos::mutex` + `std::lock_guard` / `std::unique_lock`; `paraos::*_semaphore` там, где уместно.
**Addresses:** SYNC-01, SYNC-02, SYNC-03.
**Avoids:** Pitfall 6.

### Phase 22: FreeRTOS std-like primitives hardening
**Rationale:** Некоторые API (имена потоков, `stop_token` при раннем выходе, `release(N)`) могут потребовать доработки для единообразия.
**Delivers:** Минимальные доработки `port_freertos` с тестами/примерами.
**Addresses:** FR-01, FR-02, FR-03.
**Avoids:** Pitfall 1, Pitfall 2, Pitfall 5.

### Phase 23: Build, tests and static analysis
**Rationale:** Веха завершается регрессионной защитой.
**Delivers:** PC `ctest` проходит, FreeRTOS компилируется, clang-tidy чист, стресс-тесты доступны.
**Addresses:** BLD-01, BLD-02, BLD-03, BLD-04.
**Avoids:** Pitfall 6.

## Research Flags

Phases likely needing deeper research during planning:
- **Phase 19:** нужен точный инвентарь API-различий и FreeRTOS-ограничений.
- **Phase 22:** может потребоваться spike на поведение `paraos::jthread` на FreeRTOS POSIX simulator.

Phases with standard patterns (skip research-phase):
- **Phase 20:** замена `Thread` на `jthread` уже проведена в `port_tests/test_jthread_basic.cpp`.
- **Phase 21:** `std::lock_guard<paraos::mutex>` уже проверен в `port_tests/test_mutex_basic.cpp`.
- **Phase 23:** стандартный CI-проход PC/FreeRTOS/clang-tidy.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Все std-like примитивы уже реализованы и протестированы в `port_tests/`. |
| Features | HIGH | Задача — миграция, а не создание новых возможностей. |
| Architecture | HIGH | Единая модель `paraos::jthread` + `paraos::mutex` уже утверждена. |
| Pitfalls | MEDIUM | FreeRTOS runtime на macOS остаётся ограничен; возможны сюрпризы с `join()` и именами потоков. |

**Overall confidence:** HIGH

### Gaps to Address

- **FreeRTOS runtime:** полноценный запуск FreeRTOS-тестов на macOS невозможен; ограничиться компиляцией и unit-проверками construct/destruct.
- **Имена потоков:** нужно решить, добавлять `GiveName()` в `paraos::jthread` или заменять отладочный вывод.
- **Раннее завершение задач:** нужно протестировать сценарий, когда `request_stop()` вызывается после естественного завершения callable.

## Sources

### Primary
- `port_pc/paraos_jthread.hpp` — PC `jthread` wrapper.
- `port_freertos/paraos_jthread.hpp` — FreeRTOS `jthread` implementation.
- `port_pc/paraos_mutex_std.hpp` — PC `mutex` wrapper.
- `port_freertos/paraos_mutex_std.hpp` — FreeRTOS `mutex` implementation.
- `port_pc/paraos_semaphore_std.hpp` — PC `counting_semaphore` wrapper.
- `port_freertos/paraos_semaphore_std.hpp` — FreeRTOS `counting_semaphore` implementation.
- `port_tests/test_jthread_basic.cpp` — usage pattern for `paraos::jthread`.
- `port_tests/test_mutex_basic.cpp` — usage pattern for `paraos::mutex`.
- `port_tests/test_semaphore_std.cpp` — usage pattern for `paraos::*_semaphore`.

### Secondary
- `containers/tests/test_queue_blocking_*.cpp` — legacy thread usage in queue tests.
- `containers/tests/test_multi_ringbuff_mpmc.cpp` — legacy thread usage in ring buffer test.
- `containers/tests/test_message_multithread_many_producer_many_consumers.cpp` — legacy thread usage in message buffer test.

---
*Research completed: 2026-06-22*
*Ready for roadmap: yes*
