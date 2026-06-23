# Phase 28 Context: Migrate `test_thread_only_*` sources to `paraos::jthread`

**Phase:** 28  
**Name:** Migrate `test_thread_only_*` sources to `paraos::jthread`  
**Milestone:** v1.7  
**Discussed:** 2026-06-23

## Domain

Перевод четырёх standalone-тестов `port_tests/test_thread_only_*.cpp` с legacy `paraos::Thread` на `paraos::jthread`. Цель — сохранить функциональность тестов, сделать управление потоками идиоматичным для C++ и обеспечить прохождение сборки и тестов на macOS (PC и FreeRTOS пресеты).

## Locked Decisions

### 1. Self-deleting threads → RAII vector

- `test_thread_only_stack.cpp` и `test_thread_only_stack_with_multiple_threads.cpp` переписываются на `std::vector<paraos::jthread>`.
- `new MyThreadDynamic(attr)` и `delete this` / `Finished(this)` убираются.
- Поток завершает работу выходом из лямбды/оператора; объект уничтожается при выходе из scope.
- Для `test_thread_only_stack_with_multiple_threads.cpp` синхронизация между потоками одного объекта реализуется через `paraos::binary_semaphore` или `std::atomic` флаги внутри структуры-функтора.

### 2. Static/global threads → оставить глобальными `paraos::jthread`

- `test_thread_only_static.cpp` и `test_thread_only_global.cpp`: заменить `paraos::Thread` на `paraos::jthread` в глобальных и `static`-переменных.
- Полагаться на то, что `paraos::jthread::end_scheduler()` остановит все потоки до выхода из `main()`.
- Деструкторы глобальных `paraos::jthread`, вызванные после `main()`, корректно join-ят уже завершённые потоки (семантика `std::jthread` на PC).
- Для FreeRTOS путь: `end_scheduler()` вызывается из `IdleHook`; задачи уже завершены к моменту вызова деструкторов.

### 3. Test completion tracking → stopper `paraos::jthread` + `NotifySchedulerEnded()`

- Использовать проверенный паттерн из `test_jthread_basic.cpp` и контейнерных multithread-тестов:
  - отдельный `paraos::jthread stopper` ждёт, пока счётчик завершившихся потоков достигнет ожидаемого значения;
  - `stopper` вызывает `NotifySchedulerEnded()`, который устанавливает `g_scheduler_ended = true`;
  - `IdleHook()` ожидает `g_scheduler_ended`, выполняет проверку успешности теста и вызывает `paraos::jthread::end_scheduler()`.
- Для FreeRTOS: устанавливать `paraos::freertos_idle_fnc_ptr = IdleHook;` под `#if PARAOS_LIKE_FREERTOS`.

### 4. PrintDebug synchronization → `paraos::CriticalSection`

- В макросе `PrintDebug` оставить `paraos::CriticalSection` для сериализации вывода.
- Это соответствует паттерну из `test_jthread_basic.cpp` и контейнерных тестов.
- ISR-безопасно, кроссплатформенно, без дополнительных `#ifdef`.

## Canonical Refs

- `.planning/PROJECT.md` — проектный контекст и core value.
- `.planning/REQUIREMENTS.md` — требования вехи v1.7 (MIG-01..MIG-04, LIFE-01..LIFE-03, IDIO-01).
- `.planning/ROADMAP.md` — описание Phase 28, success criteria.
- `port_tests/test_jthread_basic.cpp` — эталонный шаблон единого кроссплатформенного паттерна.
- `containers/tests/test_queue_blocking_mpmc.cpp` — пример использования `IdleHook` + `freertos_idle_fnc_ptr`.
- `containers/tests/test_message_multithread_many_producer_many_consumers.cpp` — пример stopper-потока и `NotifySchedulerEnded`.
- `port_tests/test_thread_only_stack.cpp` — исходный тест для миграции.
- `port_tests/test_thread_only_stack_with_multiple_threads.cpp` — исходный тест для миграции.
- `port_tests/test_thread_only_static.cpp` — исходный тест для миграции.
- `port_tests/test_thread_only_global.cpp` — исходный тест для миграции.
- `port_pc/paraos_jthread.hpp` — реализация `paraos::jthread` для PC.
- `port_freertos/paraos_jthread.hpp` — реализация `paraos::jthread` для FreeRTOS.

## Code Context

### Reusable patterns from existing tests

- `std::vector<paraos::jthread>` для хранения рабочих потоков.
- `std::mutex g_done_mtx; std::condition_variable g_done_cv; bool g_scheduler_ended{false};` для сигнализации о завершении теста.
- `std::atomic_size_t` счётчики для отслеживания завершения потоков.
- `paraos::stop_token` в сигнатуре рабочих лямбд/функторов.
- `paraos::sleep_for(std::chrono::milliseconds{...})` вместо `paraos::Thread::DelayMs()`.

### Anti-patterns to avoid

- `std::_Exit()` и платформенные ветви `#if defined(PARAOS_LIKE_FREERTOS) ... #else ... #endif` вокруг завершения теста.
- `paraos::Thread::StartScheduler()`, `paraos::Thread::Exit()`, `paraos::Thread::DeleteAll()`.
- Ручное `new`/`delete` потоков там, где можно использовать RAII.

## Deferred Ideas

| Idea | Why Deferred |
|------|--------------|
| Миграция `example_thread_check_timeout.cpp` на `paraos::jthread` | Вне scope Phase 28; отложено до будущей вехи (FUT-01). |
| Полный переход `extra/` на `paraos::jthread` / `paraos::mutex` | Вне scope Phase 28; отложено до будущей вехи (FUT-02). |

## Open Questions

_None — все ключевые решения зафиксированы выше._
