# Project Milestones: PARAOS

## v1.6 paraos::jthread scheduler control (Shipped: 2026-06-22)

**Delivered:** Добавлено кроссплатформенное управление планировщиком в `paraos::jthread`; FreeRTOS-порт делегирует вызовы в `vTaskStartScheduler()` / `vTaskEndScheduler()`; PC-порт эмулирует семантику FreeRTOS через гейтинг запуска и реестр активных потоков; multithread-тесты унифицированы и проходят на PC.

**Phases completed:** 24–27 (18 plans total)

**Key accomplishments:**

- Добавлены `paraos::jthread::start_scheduler()`, `end_scheduler()` и `is_scheduler_running()` в `port_freertos/paraos_jthread.hpp` с делегированием в FreeRTOS API.
- Реализован PC-порт управления планировщиком с heap-allocated контекстом, per-thread gate, статическим реестром и корректной обработкой move/join/request_stop.
- Переписан `port_tests/test_jthread_basic.cpp` без `std::_Exit()` и платформенных ветвей; используется единый паттерн со stopper-потоком.
- Обновлены пять контейнерных multithread-тестов (`test_queue_blocking_mpsc/spmc/mpmc`, `test_multi_ringbuff_mpmc`, `test_message_multithread_many_producer_many_consumers`) для явного вызова `start_scheduler()`.
- Сохранён неизменным `paraos::Thread`; `paraos::jthread` не использует legacy Thread API.
- PC-пресеты (`pc_debug_clang`, `pc_debug_gcc`) проходят `ctest` 58/58 с таймаутом 20 секунд.
- `pc_debug_gcc_clang_tidy` собирается без новых предупреждений от кода `paraos::jthread`.
- `freertos_debug_clang` и `freertos_debug_gcc` успешно компилируются.

**Stats:**

- 13 файлов изменено (код)
- 4 новых/обновлённых заголовка/теста
- 4 phases, 18 plans
- ~1 день на выполнение вехи

**Git range:** `7a3e564` → `HEAD`

**Known deferred items at close:** FreeRTOS runtime tests на macOS POSIX-симуляторе остаются ограничены средой; Windows runtime verification не проводился на macOS-хосте.

**What's next:** Определяется через `/gsd-new-milestone`.

---

## v1.5 Modernize container tests on std-like primitives (Shipped: 2026-06-22)

**Delivered:** Многопоточные тесты контейнеров в `containers/tests/` переведены с legacy-примитивов `paraos::Thread` на современные `paraos::jthread`, `paraos::mutex` и `paraos::*_semaphore`; добавлен кроссплатформенный `paraos::sleep_for`; PC-пресеты проходят 58/58 тестов, FreeRTOS-пресеты компилируются без ошибок.

**Phases completed:** 19–23 (5 plans total)

**Key accomplishments:**

- Проведён аудит пяти multithread-тестов `containers/tests/` и зафиксирована карта замены legacy-примитивов на std-like.
- Переведены `test_queue_blocking_spmc/mpsc/mpmc`, `test_multi_ringbuff_mpmc` и `test_message_multithread_many_producer_many_consumers` на `paraos::jthread` с RAII-завершением.
- Добавлены smoke-тесты `paraos::mutex` + `std::lock_guard` / `std::unique_lock`, `paraos::binary_semaphore` и `paraos::counting_semaphore` в `containers/tests/test_queue_blocking.cpp`.
- Добавлен кроссплатформенный хелпер `paraos::sleep_for(std::chrono::milliseconds)` (`paraos_sleep.hpp`) и устранена гонка в `test_message_multithread_many_producer_many_consumers`.
- Исправлены предупреждения clang-tidy в `containers/paraos_ringbuff.hpp` и `containers/paraos_multi_ringbuff.hpp`; все пресеты (`pc_debug_clang`, `pc_debug_gcc`, `freertos_debug_clang`, `freertos_debug_gcc`, `*_clang_tidy`) собираются без новых ошибок.

**Stats:**

- 11 файлов изменено
- +643 / −759 строк
- 5 phases, 5 plans
- ~2.5 часа с начала вехи до закрытия

**Git range:** `691bfdf` → `7a3e564`

**Known deferred items at close:** macOS GitLab CI runner, macOS-specific build instructions, FreeRTOS runtime tests on macOS POSIX simulator, Windows jthread runtime verification.

**What's next:** Определяется через `/gsd-new-milestone`.

---

## v1.4 std::semaphore-style Semaphore API (Shipped: 2026-06-22)

**Delivered:** Добавлен кроссплатформенный API `paraos::counting_semaphore<LeastMaxValue>` и `paraos::binary_semaphore` в стиле `std::counting_semaphore` рядом со старым `paraos::SemaphoreCounting` / `paraos::SemaphoreBinary`.

**Phases completed:** 16–18 (3 plans total)

**Key accomplishments:**

- Реализован `paraos::counting_semaphore` для PC (`port_pc/` → `port_unix/`, `port_win/`) как тонкая обёртка над `std::counting_semaphore`.
- Реализован `paraos::counting_semaphore` для FreeRTOS (`port_freertos/`) поверх FreeRTOS counting semaphore API.
- Добавлены forwarding-заголовки `port_unix/paraos_semaphore_std.hpp` и `port_win/paraos_semaphore_std.hpp`.
- Обеспечена совместимость с `std::chrono` таймаутами (`try_acquire_for`, `try_acquire_until`) на FreeRTOS.
- Добавлен тест `port_tests/test_semaphore_std.cpp` и зарегистрирован в CTest.
- PC-пресеты (`pc_debug_clang`, `pc_debug_gcc`) собираются и проходят `ctest`.
- `*_clang_tidy` пресеты не получили новых предупреждений от кода семафоров.

**Stats:**

- 16 файлов изменено
- +1758 / −31 строк
- 3 phases, 3 plans
- 1 день на выполнение вехи

**Git range:** `edf6c42` → `HEAD`

**Known deferred items at close:** macOS GitLab CI runner, macOS-specific build instructions, FreeRTOS runtime tests on macOS POSIX simulator, Windows jthread runtime verification.

**What's next:** Определяется через `/gsd-new-milestone`.

---

## v1.3 std::mutex-style Mutex API (Shipped: 2026-06-22)

**Delivered:** Добавлен кроссплатформенный API `paraos::mutex` рядом со старым `paraos::Mutex`; реализованы PC- и FreeRTOS-порты; добавлен базовый тест.

**Phases completed:** 13–15 (3 plans total)

**Key accomplishments:**

- Реализован `paraos::mutex` для PC (`port_pc/` → `port_unix/`, `port_win/`) как тонкая обёртка над `std::mutex`.
- Реализован `paraos::mutex` для FreeRTOS (`port_freertos/`) поверх FreeRTOS mutex API.
- Добавлены forwarding-заголовки `port_unix/paraos_mutex_std.hpp` и `port_win/paraos_mutex_std.hpp`.
- Обеспечена совместимость со `std::lock_guard<paraos::mutex>` и `std::unique_lock<paraos::mutex>`.
- Добавлен тест `port_tests/test_mutex_basic.cpp` и зарегистрирован в CTest.
- PC-пресеты (`pc_debug_clang`, `pc_debug_gcc`) собираются и проходят `ctest` (54/54).
- `freertos_debug_clang` и `freertos_debug_gcc` собираются; `test_mutex_basic` проходит с учётом ограничений POSIX-порта macOS.
- `*_clang_tidy` пресеты не получили новых предупреждений от кода мьютекса.

**Stats:**

- 9 файлов изменено (за вычетом planning-артефактов)
- +2 новых заголовка PC, +1 заголовок FreeRTOS, +1 тест
- 3 phases, 3 plans
- 1 день на выполнение вехи

**Git range:** `20c2f15` → `HEAD`

**Known deferred items at close:** FreeRTOS runtime-тесты с созданием задач на macOS зависят от POSIX-порта (pre-existing issue). Windows runtime не проверялся на macOS-хосте.

**What's next:** Определяется через `/gsd-new-milestone`.

---

## v1.2 std::jthread-style Thread API (Shipped: 2026-06-20)

**Delivered:** Добавлен кроссплатформенный API `paraos::jthread` рядом со старым `paraos::Thread`; минимальная версия C++ повышена до 20; PC- и FreeRTOS-порты реализованы; базовый тест добавлен и проходит на PC.

**Phases completed:** 9–12 (4 plans total)

**Key accomplishments:**

- Реализован `paraos::jthread`, `paraos::stop_token` и `paraos::stop_source` для PC (`port_pc/`) как обёртка над `std::jthread`.
- Добавлены forwarding-заголовки `port_unix/paraos_jthread.hpp` и `port_win/paraos_jthread.hpp`.
- Реализован собственный `paraos::jthread` для FreeRTOS (`port_freertos/`) с поддержкой capturing lambdas через heap-allocated invoker.
- Добавлена поддержка `ThreadAttr` (имя, стек, приоритет) в конструкторе `jthread` для всех портов.
- Повышена минимальная версия C++ до 20 в корневом `CMakeLists.txt`.
- Добавлен тест `port_tests/test_jthread_basic.cpp` и зарегистрирован в CTest.
- PC-пресеты (`pc_debug_clang`, `pc_debug_gcc`, `pc_debug_gcc_clang_tidy`) собираются и проходят `ctest`.
- `*_clang_tidy` пресеты продолжают собираться без новых предупреждений.

**Stats:**

- 22 файла изменено
- +1687 / −43 строк
- 4 phases, 4 plans
- 1 день на выполнение вехи

**Git range:** `f2c4ad1` → `20c2f15`

**Known deferred items at close:** FreeRTOS runtime-тесты с созданием задач на macOS зависают из-за ограничений POSIX-порта (pre-existing issue). Windows runtime не проверялся на macOS-хосте.

**What's next:** Определяется через `/gsd-new-milestone`.

---

## v1.1 Static Analysis Cleanup (Shipped: 2026-06-20)

**Delivered:** `*_clang_tidy` CMake presets теперь собираются на macOS без предупреждений; публичное API и поведение других платформ сохранены.

**Phases completed:** 5–8 (4 plans total)

**Key accomplishments:**

- Построены и классифицированы все предупреждения clang-tidy из пресетов `pc_debug_gcc_clang_tidy` и `freertos_debug_gcc_clang_tidy`.
- Исправлены предупреждения в core-заголовках и `port_unix/` с сохранением публичных сигнатур и семантики Linux-путей.
- Исправлены предупреждения в тестах и примерах (13 `.cpp`-файлов) с документированными inline-суппрессиями.
- Пройдена регрессионная защита: не-tidy-пресеты собираются, `port_win/` и `port_freertos/` не затронуты.

**Stats:**

- 18 файлов изменено
- +124 / −103 строк
- 4 phases, 4 plans, 14 tasks
- 1 день на выполнение вехи

**Git range:** `6d24cd5` → `f2c4ad1`

**Known deferred items at close:** 2 (macOS GitLab CI runner, macOS-specific build instructions) — см. `.planning/STATE.md` → Deferred Items.

**What's next:** v1.2 — CI/CD и документация для macOS, либо выбранная вместе с командой следующая веха.

---

## v1.0 macOS Support (Shipped: ранее)

**Delivered:** PARAOS собирается и работает на macOS; добавлена поддержка POSIX-совместимых примитивов через `port_unix/`.

See `.planning/milestones/` for full archive when available.
