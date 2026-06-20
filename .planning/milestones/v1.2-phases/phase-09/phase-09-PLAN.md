---
phase: 9
name: "PC jthread implementation"
wave: 1
depends_on: []
files_modified:
  - port_pc/paraos_jthread.hpp
  - port_unix/paraos_jthread.hpp
  - port_win/paraos_jthread.hpp
autonomous: true
---

# Plan: Phase 9 — PC jthread implementation

## Objective

Implement `paraos::jthread`, `paraos::stop_token` and `paraos::stop_source` for PC platforms (Windows + Unix) as a thin wrapper over `std::jthread`. Provide forwarding headers in `port_unix/` and `port_win/` so user code includes the same public header regardless of platform.

## Must-Haves

- [ ] MH-01: `port_pc/paraos_jthread.hpp` exists and compiles under C++20.
- [ ] MH-02: `paraos::jthread` constructs from a callable + arguments and passes `paraos::stop_token` as the last argument.
- [ ] MH-03: `paraos::stop_token` exposes `stop_requested()` and `stop_possible()`.
- [ ] MH-04: `paraos::jthread` supports `request_stop()`, `join()`, `joinable()`.
- [ ] MH-05: Destructor calls `request_stop()` + `join()` when joinable.
- [ ] MH-06: Copy is deleted; move is supported.
- [ ] MH-07: `port_unix/paraos_jthread.hpp` and `port_win/paraos_jthread.hpp` forward to `port_pc/paraos_jthread.hpp`.
- [ ] MH-08: Header follows project conventions (include guard, Doxygen, namespace paraos, no macros leaking).

## Tasks

### Task 1: Create `port_pc/paraos_jthread.hpp`

**read_first:**
- `paraos_thread_common.hpp`
- `port_unix/paraos_thread.hpp` (for style reference)

**acceptance_criteria:**
1. File created at `port_pc/paraos_jthread.hpp`.
2. Defines `namespace paraos` containing:
   - `class stop_token` wrapping `std::stop_token`.
   - `class stop_source` wrapping `std::stop_source`.
   - `class jthread` wrapping `std::jthread`.
3. `jthread` constructor forwards callable and args; appends `stop_token` as last argument via lambda/bind.
4. Copy constructor/assignment deleted; move constructor/assignment defaulted.
5. Destructor invokes `request_stop()` and `join()` when `joinable()`.
6. Public methods: `request_stop()`, `join()`, `joinable()`.

### Task 2: Create forwarding headers for Unix and Windows

**read_first:**
- `port_unix/paraos_thread.hpp`
- `port_win/paraos_thread.hpp`

**acceptance_criteria:**
1. `port_unix/paraos_jthread.hpp` includes `port_pc/paraos_jthread.hpp` only.
2. `port_win/paraos_jthread.hpp` includes `port_pc/paraos_jthread.hpp` only.
3. Both files have correct include guards and Doxygen headers.

### Task 3: Compile-check PC header

**read_first:**
- `CMakeLists.txt`

**acceptance_criteria:**
1. Configure `pc_debug_clang` preset.
2. Build the `paraos` target successfully (header-only change should compile through existing sources that include it, or via a temporary smoke compile).
3. No new clang-tidy warnings introduced.

## Verification

- `pc_debug_clang` configures and builds with the new headers present.
- A temporary smoke file including `paraos_jthread.hpp` from both Unix and Windows forwarding headers compiles under Clang with `-std=c++20`.

## Risks

- `std::jthread` requires C++20; this is addressed in Phase 12 (CMake update), but for this phase local compile checks must pass `-std=c++20`.
- `std::stop_source` may not be needed by user code yet, but exposing it keeps parity with `std::jthread` API.
