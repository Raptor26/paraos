# Phase 10: FreeRTOS jthread implementation - Context

**Gathered:** 2026-06-20
**Status:** Ready for planning
**Mode:** Auto-generated (autonomous smart discuss)

<domain>
## Phase Boundary

Implement `paraos::jthread` for FreeRTOS using FreeRTOS task API (`xTaskCreate`, `vTaskDelete`) with support for capturing lambdas and variadic arguments. The public API must match the PC implementation so user code compiles unchanged across platforms.

</domain>

<decisions>
## Implementation Decisions

### Architecture
- `port_freertos/paraos_jthread.hpp` contains a standalone `paraos::jthread` implementation (no `std::jthread`).
- A heap-allocated `Context` struct holds the invoker, stop flag, join semaphore and FreeRTOS task handle.
- A heap-allocated `InvokerBase` / `Invoker<Function, Args...>` stores the callable and arguments, allowing capturing lambdas and variadic args.
- The task entry point receives the `Context*` pointer and calls `invoker->Invoke(stop_token)`.
- `stop_token` references the `etl::atomic_bool` stop flag inside `Context`.

### Stop Token
- `paraos::stop_token` for FreeRTOS holds a pointer to `etl::atomic_bool`.
- `stop_requested()` reads the atomic flag; `stop_possible()` checks pointer non-null.
- `paraos::stop_source` is provided for parity with the PC port.

### Thread Lifecycle
- Constructor creates context, invoker and FreeRTOS task.
- `request_stop()` sets the atomic stop flag.
- `join()` waits on a binary semaphore given by the task just before it deletes itself.
- Destructor requests stop, joins, then deletes invoker and context.
- Copy is deleted; move transfers ownership of the context pointer.

### Capturing Lambdas
- Use `std::tuple<std::decay_t<Args>...>` to store arguments by value.
- Use `std::decay_t<Function>` to store the callable by value.
- Invoke via `std::apply` + `std::invoke`, appending `stop_token` as the last argument.

### Error Handling
- `xTaskCreate` failure is reported via `ETL_ASSERT` with `paraos::thread_not_created_exception`.
- Construction exceptions clean up the heap-allocated context and invoker using `gsl::finally`.

### Default Attributes
- Default stack size uses `paraos::ConvertStackSizeInWords(paraos::GetStackMinimumSizeInBytes())`.
- Default priority is `paraos::ThreadPriority::kNormal`.
- `ThreadAttr` integration is deferred to Phase 11.

### Claude's Discretion
- Exact helper class names and internal ordering are left to implementation, provided the public API and behavior match the decisions above.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `port_freertos/paraos_thread.hpp` shows FreeRTOS task creation, self-deletion and scheduler integration patterns.
- `port_freertos/paraos_semaphore.hpp` provides `paraos::SemaphoreBinary` for join synchronization.
- `paraos_thread_common.hpp` defines `ThreadAttr`, `ThreadPriority` and exception types.

### Established Patterns
- FreeRTOS tasks use `xTaskCreate` with stack size in words and `UBaseType_t` priority.
- Task self-deletion uses `vTaskDelete(nullptr)` after releasing resources.
- RAII cleanup uses `gsl::finally` and `ETL_ASSERT`.

### Integration Points
- `port_freertos/paraos_jthread.hpp` will be discovered via the existing FreeRTOS port include path.
- No changes to `port_pc/`, `port_unix/` or `port_win/` are required.

</code_context>

<specifics>
## Specific Ideas

- Context struct keeps all task-related state in one heap block to simplify move semantics.
- Stop flag uses `etl::atomic_bool` (alias for `std::atomic<bool>` in this ETL profile).
- Keep `stop_source` minimal: request_stop + get_token + stop_requested + stop_possible.

</specifics>

<deferred>
## Deferred Ideas

- ThreadAttr integration — Phase 11.
- CMake C++20 switch and tests — Phase 12.
- `std::stop_callback` compatibility — out of milestone scope.

</deferred>
