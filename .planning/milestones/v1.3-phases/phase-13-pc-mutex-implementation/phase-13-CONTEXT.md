# Phase 13: pc-mutex-implementation - Context

**Gathered:** 2026-06-22
**Status:** Ready for planning

<domain>
## Phase Boundary

Implement `paraos::mutex` for PC platforms (Windows + Unix) as a thin wrapper over `std::mutex` with a `std::mutex`-compatible API. Create a shared `port_pc/paraos_mutex_std.hpp` implementation and forwarding headers `port_unix/paraos_mutex_std.hpp` and `port_win/paraos_mutex_std.hpp` so that user code compiles unchanged across PC platforms. The new API lives alongside the existing `paraos::Mutex` / `MutexRecursive` (legacy) API; this phase does not migrate consumers.

</domain>

<decisions>
## Implementation Decisions

### API Shape
- **D-01:** `paraos::mutex` provides `lock()`, `try_lock()`, and `unlock()` with signatures and semantics matching `std::mutex`.
- **D-02:** `paraos::mutex` is non-copyable and non-movable, like `std::mutex`.
- **D-03:** Do **not** expose `native_handle()` or `native_handle_type`; keep the public API platform-agnostic and minimal.

### Error Handling
- **D-04:** If `std::mutex` construction throws `std::system_error`, let it propagate. Do not wrap it in `paraos::exception` and do not convert it to an assertion failure.

### File Layout
- **D-05:** New public header name is `paraos_mutex_std.hpp` for all ports.
  - `port_pc/paraos_mutex_std.hpp` contains the shared PC implementation over `std::mutex`.
  - `port_unix/paraos_mutex_std.hpp` forwards to `../port_pc/paraos_mutex_std.hpp`.
  - `port_win/paraos_mutex_std.hpp` forwards to `../port_pc/paraos_mutex_std.hpp`.
  - `port_freertos/paraos_mutex_std.hpp` will receive its own implementation in Phase 14.
- **D-06:** Use include guard `PARAOS_MUTEX_STD_HPP` for the new header(s) to avoid collision with the existing `PARAOS_MUTEX_HPP` guard used by the legacy `paraos_mutex.hpp` headers.

### Public Include Path
- **D-07:** Users include the platform-specific forwarding header (e.g., `port_unix/paraos_mutex_std.hpp`), matching the existing `paraos_jthread.hpp` pattern. No root-level aggregator header is added.

### Legacy Coexistence
- **D-08:** Leave existing `port_*/paraos_mutex.hpp` (legacy `paraos::Mutex` / `MutexRecursive`) untouched. The new `paraos::mutex` is a separate type in a separate header.

### Claude's Discretion
- Exact internal member naming and inline documentation style are left to implementation, provided the public API and rule-of-five behavior match the decisions above.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Requirements
- `.planning/REQUIREMENTS.md` — v1.3 mutex requirements MUTEX-01..09, MUTEX-11, BUILD-01.
- `.planning/ROADMAP.md` § "Phase 13: PC mutex implementation" — goal, success criteria, plan boundaries.

### Existing Code Patterns
- `port_pc/paraos_jthread.hpp` — reference for PC "thin wrapper over standard C++" pattern and platform-specific forwarding headers.
- `port_unix/paraos_jthread.hpp` — reference for Unix forwarding header structure.
- `port_win/paraos_jthread.hpp` — reference for Windows forwarding header structure.
- `port_unix/paraos_mutex.hpp` — legacy Unix mutex implementation and include guard pattern to avoid.
- `port_win/paraos_mutex.hpp` — legacy Windows mutex implementation and include guard pattern to avoid.
- `paraos_mutex_raii.hpp` — existing RAII wrapper over `MutexBase` (legacy); new API must not break it.
- `CMakeLists.txt` — port selection and include-directory propagation.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `port_pc/` already hosts shared PC abstractions (`port_pc/paraos_jthread.hpp`).
- `std::mutex` and `<mutex>` from C++20 provide the desired backend for PC.
- `std::lock_guard` and `std::unique_lock` are standard and require only `lock()`, `try_lock()`, `unlock()` and the BasicLockable / Lockable concepts.

### Established Patterns
- PC ports implement a single header in `port_pc/` and forward from `port_unix/` and `port_win/` via relative `#include`.
- Public headers use include guard `PARAOS_<NAME>_HPP`; because `PARAOS_MUTEX_HPP` is already taken by the legacy header, the new header must use a distinct guard.
- Legacy primitives (`Mutex`, `MutexRecursive`, `MutexBase`) remain untouched when adding new APIs, as done with `jthread` alongside `Thread`.

### Integration Points
- `port_pc/paraos_mutex_std.hpp` will be added next to `port_pc/paraos_jthread.hpp`.
- `port_unix/CMakeLists.txt` and `port_win/CMakeLists.txt` already add their directories to `target_include_directories`; no CMake changes are required for forwarding headers.
- No changes to `extra/`, `containers/`, or legacy consumers in this phase.

</code_context>

<specifics>
## Specific Ideas

- Use a private `std::mutex mutex_` member and forward `lock()` / `try_lock()` / `unlock()` directly to it.
- Explicitly default the destructor and delete copy/move operations to document the rule of five.
- Add Doxygen comments with `@file`, `@brief`, and method documentation matching the existing style.

</specifics>

<deferred>
## Deferred Ideas

- FreeRTOS implementation of `paraos::mutex` — Phase 14.
- Tests and static analysis verification — Phase 15.
- Migration of legacy `paraos::Mutex` / `MutexRecursive` consumers — future milestone, out of scope for v1.3.
- `paraos::recursive_mutex` / `paraos::timed_mutex` — v2 requirements MUTEX-12 / MUTEX-13, out of scope.

</deferred>

---

*Phase: 13-pc-mutex-implementation*
*Context gathered: 2026-06-22*
