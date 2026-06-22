# Phase 14: freertos-mutex-implementation - Context

**Gathered:** 2026-06-22
**Status:** Ready for planning

<domain>
## Phase Boundary

Implement `paraos::mutex` for FreeRTOS as a separate header `port_freertos/paraos_mutex_std.hpp` with the same public API as the PC version (`lock()`, `try_lock()`, `unlock()`), backed by the FreeRTOS mutex semaphore API. The legacy `paraos::Mutex` / `MutexRecursive` API in `port_freertos/paraos_mutex.hpp` remains untouched.

</domain>

<decisions>
## Implementation Decisions

### API Shape
- **D-01:** `paraos::mutex` provides `lock()`, `try_lock()`, and `unlock()` with the same signatures and semantics as the PC implementation.
- **D-02:** `paraos::mutex` is non-copyable and non-movable.
- **D-03:** Do not expose `native_handle()` or `native_handle_type`.

### Error Handling
- **D-04:** Constructor creates the mutex with `xSemaphoreCreateMutex()` and asserts non-null result using `ETL_ASSERT(handle_ != nullptr, std::bad_alloc());`, matching the legacy FreeRTOS mutex creation pattern.

### File Layout
- **D-05:** Public header name is `paraos_mutex_std.hpp`.
  - `port_freertos/paraos_mutex_std.hpp` contains the FreeRTOS implementation.
- **D-06:** Use include guard `PARAOS_FREERTOS_MUTEX_STD_HPP`.

### Public Include Path
- **D-07:** Users include `paraos_mutex_std.hpp`; the build system adds the active port directory to the include path.

### Legacy Coexistence
- **D-08:** Leave existing `port_freertos/paraos_mutex.hpp` (legacy `paraos::Mutex` / `MutexRecursive`) untouched.

</decisions>

<canonical_refs>
## Canonical References

### Requirements
- `.planning/REQUIREMENTS.md` — v1.3 mutex requirements MUTEX-10.
- `.planning/ROADMAP.md` § "Phase 14: FreeRTOS mutex implementation" — goal, success criteria.

### Existing Code Patterns
- `port_freertos/paraos_mutex.hpp` — legacy FreeRTOS mutex creation, take, give patterns.
- `port_pc/paraos_mutex_std.hpp` — PC `paraos::mutex` API shape to mirror.
- `port_freertos/paraos_jthread.hpp` — FreeRTOS error handling pattern with `ETL_ASSERT`.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- FreeRTOS headers `FreeRTOS.h` and `semphr.h`.
- `SemaphoreHandle_t`, `xSemaphoreCreateMutex`, `xSemaphoreTake`, `xSemaphoreGive`, `portMAX_DELAY`.
- `ETL_ASSERT` for constructor validation.

### Integration Points
- `port_freertos/paraos_mutex_std.hpp` is added next to `port_freertos/paraos_jthread.hpp`.
- No CMake changes are required; the port directory is already on the include path.

</code_context>

<specifics>
## Specific Ideas

- Store `SemaphoreHandle_t handle_` and delete it in the destructor with `vSemaphoreDelete(handle_)`.
- `lock()`: `xSemaphoreTake(handle_, portMAX_DELAY)`.
- `try_lock()`: return `xSemaphoreTake(handle_, 0) == pdTRUE`.
- `unlock()`: `xSemaphoreGive(handle_)`.
- Delete copy/move and default destructor (with `vSemaphoreDelete`).

</specifics>

<deferred>
## Deferred Ideas

- Tests and static analysis verification — Phase 15.
- Migration of legacy consumers — future milestone.

</deferred>

---

*Phase: 14-freertos-mutex-implementation*
*Context gathered: 2026-06-22*
