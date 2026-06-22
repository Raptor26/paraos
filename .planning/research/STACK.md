# Research: Stack — std::mutex-style mutex for PARAOS

**Milestone:** v1.3 std::mutex-style Mutex API
**Dimension:** Stack additions/changes needed for `paraos::mutex`.

## Existing stack

- C++20 minimum (already in place after v1.2).
- `port_pc/` shared header for Unix/Windows PC implementations (established by v1.2 `paraos_jthread.hpp`).
- Forwarding headers in `port_unix/` and `port_win/` that include `port_pc/` (established by v1.2).
- FreeRTOS backend uses `semphr.h` (`xSemaphoreCreateMutex`, `xSemaphoreCreateRecursiveMutex`, `xSemaphoreTake`, `xSemaphoreGive`, `xSemaphoreTakeRecursive`, `xSemaphoreGiveRecursive`, ISR variants).
- POSIX backend uses `<pthread.h>`.
- WinAPI backend uses `<windows.h>` (`CreateMutex`, `WaitForSingleObject`, `ReleaseMutex`).

## Stack additions needed

### PC (`port_pc/`)

- Include `<mutex>` only — no new dependencies.
- Use `std::mutex` as the sole storage and implementation.
- Provide `lock()`, `try_lock()`, `unlock()` that forward to `std::mutex`.

### Unix (`port_unix/`)

- Forwarding header `#include "../port_pc/paraos_mutex.hpp"`.
- No new platform-specific code because PC header already compiles on Linux and macOS.

### Windows (`port_win/`)

- Forwarding header `#include "../port_pc/paraos_mutex.hpp"`.
- No new platform-specific code.

### FreeRTOS (`port_freertos/`)

- Use existing FreeRTOS mutex/semaphore API already used by `paraos::Mutex`.
- No new FreeRTOS features or config options required.

## What NOT to add

- No new third-party libraries.
- No changes to CMake toolchain files.
- No changes to `paraos::Mutex` internals.
- No new compile definitions or macros beyond existing port macros.
