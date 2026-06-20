# Requirements: v1.2 std::jthread-style Thread API

## Scope

Добавить в PARAOS новый публичный API `paraos::jthread`, совместимый по духу с `std::jthread` из C++20. API должен быть единым для Windows, Unix и FreeRTOS: пользовательский код, использующий `paraos::jthread`, компилируется без изменений под любой платформой.

Старый `paraos::Thread` остаётся нетронутым — замена поэтапная. Новый API вводится рядом со старым.

## Requirements

### JTHREAD — Core API

- [ ] **JTHREAD-01**: `paraos::jthread` конструируется из callable и произвольного числа аргументов.
  - `paraos::stop_token` передаётся в callable последним аргументом.
  - Поддерживаются capturing lambdas, free functions, member functions через `std::function`-подобный механизм на FreeRTOS и `std::jthread` на PC.
- [ ] **JTHREAD-02**: `paraos::stop_token` предоставляет `stop_requested()` и `stop_possible()`.
- [ ] **JTHREAD-03**: `paraos::jthread::request_stop()` запрашивает остановку потока.
- [ ] **JTHREAD-04**: `paraos::jthread::join()` блокирует вызывающий поток до завершения целевого потока.
- [ ] **JTHREAD-05**: Деструктор `paraos::jthread` автоматически вызывает `request_stop()` и `join()`, если поток joinable.
- [ ] **JTHREAD-06**: `paraos::jthread` поддерживает move-семантику, но не копирование.

### JTHREAD — Thread Attributes

- [ ] **JTHREAD-07**: Конструктор `paraos::jthread` принимает `ThreadAttr` (имя, размер стека, приоритет) наряду с callable и аргументами.
- [ ] **JTHREAD-08**: На FreeRTOS `ThreadAttr::priority` задаёт приоритет задачи FreeRTOS.
- [ ] **JTHREAD-09**: На PC `ThreadAttr::priority` обрабатывается через платформенный API (`pthread_setschedparam` на Unix, `SetThreadPriority` на Windows) или игнорируется, если платформа не позволяет его задать без привилегий.
- [ ] **JTHREAD-10**: На FreeRTOS `ThreadAttr::stack_depth` и `thread_name` передаются в `xTaskCreate`.

### Portability

- [ ] **PORT-01**: `port_pc/paraos_jthread.hpp` содержит единую реализацию `paraos::jthread` для Windows и Unix поверх `std::jthread`.
- [ ] **PORT-02**: `port_unix/paraos_jthread.hpp` включает `port_pc/paraos_jthread.hpp`.
- [ ] **PORT-03**: `port_win/paraos_jthread.hpp` включает `port_pc/paraos_jthread.hpp`.
- [ ] **PORT-04**: `port_freertos/paraos_jthread.hpp` содержит собственную реализацию `paraos::jthread` поверх FreeRTOS API.
- [ ] **PORT-05**: Пользовательский код не должен содержать платформенных `#ifdef` для выбора между `jthread`-реализациями.

### Build & Tests

- [ ] **BUILD-01**: Минимальная версия C++ повышена до 20 в корневом `CMakeLists.txt`.
- [ ] **BUILD-02**: `paraos_jthread.hpp` доступен для включения из всех портов.
- [ ] **TEST-01**: Тест `port_tests/test_jthread_basic.cpp` компилируется и проходит на `pc_debug_clang`.
- [ ] **TEST-02**: Тест `port_tests/test_jthread_basic.cpp` компилируется и проходит на `pc_debug_gcc`.
- [ ] **TEST-03**: Тест `port_tests/test_jthread_basic.cpp` компилируется и проходит на `freertos_debug_clang`.
- [ ] **TEST-04**: Тест `port_tests/test_jthread_basic.cpp` компилируется и проходит на `freertos_debug_gcc`.
- [ ] **TEST-05**: `*_clang_tidy` пресеты продолжают собираться без новых предупреждений.

## Out of Scope

- Полная замена `paraos::Thread` на `paraos::jthread` — в этой вехе новый API добавляется рядом со старым.
- Миграция `extra/` и `containers/` на `jthread` — откладывается на будущие вехи.
- Добавление новых портов (macOS, Zephyr и т.д.) — в этой вехе поддерживаются только Windows, Unix, FreeRTOS.
- `std::stop_callback`-совместимый API — в минимальной версии не требуется.
- Полная бинарная совместимость с `std::jthread` (например, `get_id`, `detach`, `hardware_concurrency`) — только то, что нужно для целевого теста и базового использования.

## Traceability

| Requirement | Phase | Plan |
|-------------|-------|------|
| JTHREAD-01  | TBD   | TBD  |
| JTHREAD-02  | TBD   | TBD  |
| JTHREAD-03  | TBD   | TBD  |
| JTHREAD-04  | TBD   | TBD  |
| JTHREAD-05  | TBD   | TBD  |
| JTHREAD-06  | TBD   | TBD  |
| JTHREAD-07  | TBD   | TBD  |
| JTHREAD-08  | TBD   | TBD  |
| JTHREAD-09  | TBD   | TBD  |
| JTHREAD-10  | TBD   | TBD  |
| PORT-01     | TBD   | TBD  |
| PORT-02     | TBD   | TBD  |
| PORT-03     | TBD   | TBD  |
| PORT-04     | TBD   | TBD  |
| PORT-05     | TBD   | TBD  |
| BUILD-01    | TBD   | TBD  |
| BUILD-02    | TBD   | TBD  |
| TEST-01     | TBD   | TBD  |
| TEST-02     | TBD   | TBD  |
| TEST-03     | TBD   | TBD  |
| TEST-04     | TBD   | TBD  |
| TEST-05     | TBD   | TBD  |
