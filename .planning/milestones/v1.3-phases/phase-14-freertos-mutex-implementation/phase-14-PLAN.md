# Phase 14: freertos-mutex-implementation - Plan

**Plan ID:** 14-01  
**Phase:** 14  
**Created:** 2026-06-22  
**Status:** Ready for execution

## Goal

Implement `paraos::mutex` for FreeRTOS as a self-contained header backed by the FreeRTOS mutex semaphore API.

## Scope

- Create `port_freertos/paraos_mutex_std.hpp`.
- Match the PC `paraos::mutex` public API (`lock()`, `try_lock()`, `unlock()`).
- Leave legacy `paraos::Mutex` / `MutexRecursive` untouched.
- Do not add tests in this phase (Phase 15 owns tests).

## Requirements Addressed

- MUTEX-10: `port_freertos/paraos_mutex_std.hpp` содержит собственную реализацию `paraos::mutex` поверх FreeRTOS mutex API.

## Tasks

### Task 1: Create FreeRTOS implementation

**File:** `port_freertos/paraos_mutex_std.hpp`

- Define include guard `PARAOS_FREERTOS_MUTEX_STD_HPP`.
- Include `FreeRTOS.h`, `semphr.h`, `etl/error_handler.h`.
- Define `namespace paraos { class mutex; }`.
- Provide:
  - `void lock()` — `xSemaphoreTake(handle_, portMAX_DELAY)`.
  - `bool try_lock()` — `return xSemaphoreTake(handle_, 0) == pdTRUE;`.
  - `void unlock()` — `xSemaphoreGive(handle_)`.
- Constructor: `handle_ = xSemaphoreCreateMutex(); ETL_ASSERT(handle_ != nullptr, std::bad_alloc());`.
- Destructor: `if (handle_ != nullptr) { vSemaphoreDelete(handle_); }`.
- Delete copy/move constructors and assignments.
- Add Doxygen comments matching project style.

## Verification

- Configure and build `freertos_debug_clang` and `freertos_debug_gcc` presets.
- Confirm a minimal compile-only snippet using `std::lock_guard<paraos::mutex>` compiles against the FreeRTOS build.
- Confirm no new warnings originate from `port_freertos/paraos_mutex_std.hpp`.

## Risks

- FreeRTOS runtime tests cannot run on macOS POSIX simulator (documented environment limitation), so only compile-time verification is possible here.
