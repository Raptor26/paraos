# Research Summary: std::mutex-style Mutex API

**Milestone:** v1.3 std::mutex-style Mutex API
**Synthesized:** 2026-06-22

## Stack additions

- No new third-party dependencies.
- `port_pc/paraos_mutex.hpp` uses only `<mutex>`.
- `port_unix/` and `port_win/` reuse the PC header via forwarding include.
- FreeRTOS header reuses existing `semphr.h` mutex API.

## Feature table stakes

- `lock()`, `try_lock()`, `unlock()` with exact signatures required for `std::BasicLockable`.
- Non-copyable, non-movable (match `std::mutex`).
- Compatible with `std::lock_guard`, `std::unique_lock`, `std::scoped_lock`.

## Watch out for

1. Signature mismatch with `std::mutex` breaks RAII wrappers.
2. Do not make `paraos::mutex` movable — locked mutex must not be moved.
3. FreeRTOS mutex creation can fail; assert on `nullptr` consistently.
4. Keep `paraos::mutex` separate from existing `paraos::Mutex`; no implicit conversions.
5. clang-tidy `WarningsAsErrors: '*'` requires clean, minimal header code.
6. FreeRTOS POSIX simulator on macOS may hang runtime tests; terminate from scheduler.
7. Remember to register the new test in CMake with clang-tidy properties.

## Integration outline

```
port_pc/paraos_mutex.hpp      → std::mutex wrapper
port_unix/paraos_mutex.hpp    → #include "../port_pc/paraos_mutex.hpp"
port_win/paraos_mutex.hpp     → #include "../port_pc/paraos_mutex.hpp"
port_freertos/paraos_mutex.hpp → xSemaphoreCreateMutex wrapper
port_tests/test_mutex_basic.cpp → lock_guard/unique_lock tests
```
