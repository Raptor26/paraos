# Phase 11: Thread attributes integration - Context

**Gathered:** 2026-06-20
**Status:** Ready for planning
**Mode:** Auto-generated (autonomous smart discuss)

<domain>
## Phase Boundary

Add a `paraos::jthread` constructor overload that accepts `paraos::ThreadAttr` (name, stack depth, priority) alongside the callable and arguments. Implement attribute handling for PC (Windows + Unix) and FreeRTOS ports while preserving the existing `paraos::ThreadAttr` API.

</domain>

<decisions>
## Implementation Decisions

### API Shape
- Add `template<typename Function, typename... Args> jthread(const ThreadAttr& attr, Function&& f, Args&&... args)` to all ports.
- The existing `jthread(Function&& f, Args&&... args)` constructor delegates to the ThreadAttr overload with a default `ThreadAttr{}`.
- Backward compatibility: `paraos::ThreadAttr` is not modified; `dtor_callback` and `run_` fields are simply ignored by `jthread`.

### PC Port
- `port_pc/paraos_jthread.hpp` receives the ThreadAttr overload and platform-specific `ApplyAttr()` helper.
- Unix: attempt `pthread_setschedparam(native_handle(), SCHED_RR, mapped_priority)`; ignore privilege failures to match existing `paraos::Thread` behavior.
- Windows: attempt `SetThreadPriority(native_handle(), attr.priority)`.
- `thread_name` and `stack_depth` are stored in the `jthread` object but have no effect on `std::jthread`; this is documented.

### FreeRTOS Port
- `port_freertos/paraos_jthread.hpp` receives the ThreadAttr overload.
- `xTaskCreate` uses `attr.thread_name.data()`, `ConvertStackSizeInWords(attr.stack_depth)` and `static_cast<UBaseType_t>(attr.priority)`.
- Default constructor uses a default `ThreadAttr{}`.

### Priority Mapping
- Unix/macOS reuses the existing `MapPriorityToSchedRange` approach from `port_unix/paraos_thread.hpp`.
- Windows uses `ThreadPriority` enum values directly (they are WinAPI constants).
- FreeRTOS uses `ThreadPriority` enum values directly (0..6).

### Claude's Discretion
- Exact internal helper naming and error handling for privileged priority failures are left to implementation, provided behavior matches existing `paraos::Thread` patterns.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `paraos_thread_common.hpp` defines `ThreadAttr` and `ThreadPriority`.
- `port_unix/paraos_thread.hpp` shows Unix priority mapping and privilege handling.
- `port_win/paraos_thread.hpp` shows Windows priority enum values.
- `port_freertos/paraos_thread.hpp` shows `xTaskCreate` usage with ThreadAttr.

### Established Patterns
- Unix priority is mapped to `SCHED_RR` range; if not root, setting is silently accepted for backward compatibility.
- FreeRTOS task creation uses stack size in words and `UBaseType_t` priority.
- Platform-specific code is isolated with `PARAOS_LIKE_UNIX` / `PARAOS_LIKE_WINAPI` macros inside shared PC headers.

### Integration Points
- All three jthread headers (`port_pc`, `port_freertos`) receive the new constructor overload.
- No changes to `paraos::Thread` consumers.

</code_context>

<specifics>
## Specific Ideas

- Keep `ApplyAttr()` private and split by platform with `#ifdef`.
- Store `ThreadAttr` as a member in PC `jthread` for introspection.
- On FreeRTOS, keep the stored `ThreadAttr` minimal or just pass fields directly to `xTaskCreate`.

</specifics>

<deferred>
## Deferred Ideas

- CMake integration, C++20 switch and tests — Phase 12.
- `std::stop_callback` compatibility — out of milestone scope.

</deferred>
