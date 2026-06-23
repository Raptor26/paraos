# Discussion Log: Phase 28

**Phase:** 28 — Migrate `test_thread_only_*` sources to `paraos::jthread`  
**Milestone:** v1.7  
**Date:** 2026-06-23

---

## Area 1: Self-deleting threads — heap + delete vs RAII vector

**Question:** Как поступить с self-deleting потоками в `test_thread_only_stack.cpp` и `test_thread_only_stack_with_multiple_threads.cpp`?

**Options presented:**
- A. Сохранить self-delete паттерн (`new` + `delete this`).
- B. Переписать на `std::vector<paraos::jthread>` — RAII, выход из лямбды, уничтожение при выходе из scope.
- C. Комбинированный вариант: один тест на RAII, другой оставить с self-delete.

**Selected:** B

**Notes:** Убрать `new MyThreadDynamic(attr)` и `delete this`. Для `test_thread_only_stack_with_multiple_threads.cpp` синхронизация между потоками одного объекта — через `paraos::binary_semaphore` или `std::atomic` внутри функтора.

---

## Area 2: Static/global threads — `std::optional` / `std::unique_ptr` vs global raw `jthread`

**Question:** Как управлять глобальными/статическими потоками в `test_thread_only_static.cpp` и `test_thread_only_global.cpp`?

**Options presented:**
- A. Оставить глобальными `paraos::jthread`, полагаясь на `end_scheduler()`.
- B. Оборачивать в `std::optional<paraos::jthread>` для явного управления временем жизни.
- C. Использовать `std::unique_ptr<paraos::jthread>`.

**Selected:** A

**Rationale:** `end_scheduler()` корректно остановит все потоки до выхода из `main()`. Деструкторы глобальных `std::jthread`-объектов после `main()` вызовут `join()` на уже завершённых потоках — это корректная семантика стандартной библиотеки C++. Для FreeRTOS `end_scheduler()` вызывается из `IdleHook`.

---

## Area 3: Test completion tracking — stopper jthread vs `std::latch` / барьер

**Question:** Чем заменить watcher-поток `check_test_complete_and_exit`, который вызывал `Thread::Exit()`?

**Options presented:**
- A. Stopper `paraos::jthread` + `NotifySchedulerEnded()` (паттерн из `test_jthread_basic.cpp`).
- B. `std::latch`.
- C. `std::atomic` + `std::condition_variable`.

**Selected:** A

**Rationale:** Stopper `jthread` — единственный вариант, совместимый с FreeRTOS-путём, где `start_scheduler()` не возвращает управление. Паттерн уже проверен в проекте.

---

## Area 4: PrintDebug synchronization — `paraos::CriticalSection` vs `std::mutex`

**Question:** Какой примитив использовать в макросе `PrintDebug`?

**Options presented:**
- A. `paraos::CriticalSection`.
- B. `std::mutex` / `std::scoped_lock`.
- C. Условная компиляция PC/FreeRTOS.

**Selected:** A

**Rationale:** `paraos::CriticalSection` используется в `test_jthread_basic.cpp` и контейнерных тестах, ISR-безопасен и кроссплатформенен. Не добавляет `#ifdef`.

---

## Deferred Ideas

- Миграция `example_thread_check_timeout.cpp` на `paraos::jthread` — отложена до будущей вехи (FUT-01).
- Полный переход `extra/` на `paraos::jthread` / `paraos::mutex` — отложен до будущей вехи (FUT-02).

---

## Next Step

`/gsd-plan-phase 28` — создать план реализации Phase 28 на основе зафиксированных решений.
