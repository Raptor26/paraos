---
phase: 10
name: "FreeRTOS jthread implementation"
wave: 1
depends_on: ["9"]
files_modified:
  - port_freertos/paraos_jthread.hpp
autonomous: true
---

# Plan: Phase 10 — FreeRTOS jthread implementation

## Objective

Implement `paraos::jthread`, `paraos::stop_token` and `paraos::stop_source` for FreeRTOS without relying on `std::jthread`, supporting capturing lambdas and variadic arguments through a heap-allocated invoker.

## Must-Haves

- [ ] MH-01: `port_freertos/paraos_jthread.hpp` exists and compiles under C++17/20.
- [ ] MH-02: `paraos::jthread` constructs from callable + args; `stop_token` is passed as the last argument.
- [ ] MH-03: `paraos::stop_token` exposes `stop_requested()` and `stop_possible()`.
- [ ] MH-04: `paraos::stop_source` exposes `request_stop()`, `get_token()`, `stop_requested()`, `stop_possible()`.
- [ ] MH-05: `paraos::jthread` supports `request_stop()`, `join()`, `joinable()`.
- [ ] MH-06: Destructor requests stop and joins when joinable.
- [ ] MH-07: Copy is deleted; move is supported.
- [ ] MH-08: Capturing lambdas and arbitrary argument types are supported via heap-allocated invoker.
- [ ] MH-09: Task creation uses FreeRTOS `xTaskCreate`; task self-deletes with `vTaskDelete(nullptr)`.
- [ ] MH-10: Header follows project conventions (include guard, Doxygen, namespace paraos).

## Tasks

### Task 1: Implement `port_freertos/paraos_jthread.hpp`

**read_first:**
- `port_freertos/paraos_thread.hpp`
- `port_freertos/paraos_semaphore.hpp`
- `paraos_thread_common.hpp`
- `port_pc/paraos_jthread.hpp`

**acceptance_criteria:**
1. File created at `port_freertos/paraos_jthread.hpp`.
2. Defines `paraos::stop_token` backed by a pointer to `etl::atomic_bool`.
3. Defines `paraos::stop_source` that controls an `etl::atomic_bool`.
4. Defines `paraos::jthread` with:
   - Heap-allocated `Context` holding invoker, stop flag, join semaphore and task handle.
   - Heap-allocated `InvokerBase` / `Invoker<Function, Args...>` storing callable and arguments by value.
   - Constructor that creates task via `xTaskCreate`.
   - `request_stop()`, `join()`, `joinable()`.
   - Deleted copy, defaulted/deleted move as appropriate.
   - Destructor that stops, joins and frees resources.
5. Task entry point invokes the callable with provided args + `stop_token` last, then signals join semaphore and self-deletes.

### Task 2: Compile-check FreeRTOS header

**read_first:**
- `CMakeLists.txt`
- `CMakePresets.json`

**acceptance_criteria:**
1. Configure `freertos_debug_gcc` preset.
2. Build the `paraos` target successfully.
3. No new warnings or errors introduced.

## Verification

- `freertos_debug_gcc` configures and builds with the new header present.
- A temporary smoke file including `paraos_jthread.hpp` under FreeRTOS defines compiles with the FreeRTOS GCC toolchain.

## Risks

- FreeRTOS task stack may be too small for C++ exceptions/virtual calls; default stack is minimal until ThreadAttr integration in Phase 11.
- `std::tuple`/`std::apply` require C++17; this is already the project baseline.
- Move semantics must keep the task's `Context*` valid; context is heap-allocated and never relocated.
