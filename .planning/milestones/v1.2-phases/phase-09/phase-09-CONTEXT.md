# Phase 9: PC jthread implementation - Context

**Gathered:** 2026-06-20
**Status:** Ready for planning
**Mode:** Auto-generated (autonomous smart discuss)

<domain>
## Phase Boundary

Implement `paraos::jthread` for Windows and Unix as a thin wrapper over `std::jthread`, providing a unified cross-platform thread API. This phase delivers the PC port only; FreeRTOS port comes in Phase 10. The public API must not require platform `#ifdef` in user code.

</domain>

<decisions>
## Implementation Decisions

### API Shape
- `paraos::jthread` lives in `port_pc/paraos_jthread.hpp` and is included by `port_unix/paraos_jthread.hpp` and `port_win/paraos_jthread.hpp`.
- `paraos::jthread` wraps `std::jthread`; `paraos::stop_token` wraps `std::stop_token`.
- Constructor signature: `jthread(Function&& f, Args&&... args)`. The callable receives `stop_token` as its last argument.
- Copy operations are deleted; move operations are defaulted or implemented to transfer the underlying `std::jthread`.
- Destructor calls `request_stop()` then `join()` when `joinable()`.
- `request_stop()`, `join()`, `joinable()` delegate directly to `std::jthread`.

### Platform Headers
- `port_unix/paraos_jthread.hpp` and `port_win/paraos_jthread.hpp` are simple include-forwarding headers to `port_pc/paraos_jthread.hpp`.
- No separate `port_pc/CMakeLists.txt` is created; the new header is added to the existing `paraos` target include paths via root `CMakeLists.txt` in Phase 12.

### Namespace and Naming
- All public symbols live in `namespace paraos`.
- Type aliases or thin wrappers keep the public surface compatible with future FreeRTOS implementation.
- No attempt is made to exactly replicate every `std::jthread` member (e.g., `get_id`, `detach`, `hardware_concurrency`); only required members are exposed.

### Thread Attributes
- `ThreadAttr` integration is explicitly deferred to Phase 11; the Phase 9 constructor accepts only callable + args.
- The existing `paraos::ThreadAttr` from `paraos_thread_common.hpp` remains untouched.

### Error Handling
- `std::jthread` exceptions propagate naturally; no custom exception wrapping is added for PC.
- FreeRTOS-specific error handling will be designed in Phase 10.

### Claude's Discretion
- Exact internal implementation details (whether to use inheritance, composition, or type aliases) are left to implementation, provided the public API and behavior match the decisions above.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `paraos_thread_common.hpp` defines `ThreadAttr`, `ThreadPriority`, and `thread_delegate_type`.
- Existing `paraos::Thread` in `port_unix/paraos_thread.hpp` and `port_win/paraos_thread.hpp` provides reference patterns for priority and naming, but the new `jthread` API is intentionally distinct.

### Established Patterns
- Public headers use `paraos_` prefix, include guards `PARAOS_<NAME>_HPP`, Doxygen `///` comments, and live in `namespace paraos`.
- PC-specific shared code is placed in `port_pc/` (does not exist yet; will be created).
- Unix and Windows ports forward to PC shared headers when semantics align.

### Integration Points
- Root `CMakeLists.txt` adds `port_pc/` to include directories in a later phase.
- New test `port_tests/test_jthread_basic.cpp` will be added in Phase 12.
- No changes to existing `paraos::Thread` consumers in `extra/` or `containers/`.

</code_context>

<specifics>
## Specific Ideas

- Keep the PC implementation minimal: `paraos::jthread` can hold a `std::jthread` member and expose the required operations.
- Use `std::forward` and perfect forwarding for constructor arguments.
- Use `std::bind` or a lambda to append `stop_token` as the last argument to the user callable.

</specifics>

<deferred>
## Deferred Ideas

- `ThreadAttr` constructor overload — Phase 11.
- FreeRTOS implementation — Phase 10.
- Tests and CMake integration — Phase 12.
- `std::stop_callback` compatibility — out of milestone scope.

</deferred>
