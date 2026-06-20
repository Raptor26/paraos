---
phase: 11
name: "Thread attributes integration"
wave: 1
depends_on: ["10"]
files_modified:
  - port_pc/paraos_jthread.hpp
  - port_freertos/paraos_jthread.hpp
autonomous: true
---

# Plan: Phase 11 — Thread attributes integration

## Objective

Add a `paraos::ThreadAttr` accepting constructor overload to `paraos::jthread` on PC and FreeRTOS ports, mapping name, stack depth and priority to the underlying platform APIs.

## Must-Haves

- [ ] MH-01: `paraos::jthread` has a constructor `jthread(const ThreadAttr& attr, Function&& f, Args&&... args)` on PC and FreeRTOS.
- [ ] MH-02: The existing `jthread(Function&& f, Args&&... args)` constructor delegates to the ThreadAttr overload with default attributes.
- [ ] MH-03: FreeRTOS `xTaskCreate` uses `attr.thread_name`, `attr.stack_depth` and `attr.priority`.
- [ ] MH-04: Unix PC attempts to set thread priority via `pthread_setschedparam` with `SCHED_RR` and mapped priority.
- [ ] MH-05: Windows PC attempts to set thread priority via `SetThreadPriority`.
- [ ] MH-06: Privilege failures on Unix are ignored (backward compatibility), matching `paraos::Thread` behavior.
- [ ] MH-07: Existing `paraos::ThreadAttr` is unchanged.
- [ ] MH-08: PC and FreeRTOS smoke tests compile and pass.
- [ ] MH-09: Existing presets continue to build without regressions.

## Tasks

### Task 1: Add ThreadAttr constructor to PC jthread

**read_first:**
- `port_pc/paraos_jthread.hpp`
- `port_unix/paraos_thread.hpp`
- `port_win/paraos_thread.hpp`
- `paraos_thread_common.hpp`

**acceptance_criteria:**
1. `port_pc/paraos_jthread.hpp` defines a private `ApplyAttr(const ThreadAttr&)` method.
2. Unix branch maps priority and calls `pthread_setschedparam`; ignores non-root failures.
3. Windows branch calls `SetThreadPriority(native_handle(), static_cast<int>(attr.priority))`.
4. A new public constructor `jthread(const ThreadAttr&, Function&&, Args&&...)` is added.
5. The old constructor delegates to the new one with `ThreadAttr{}`.
6. `ThreadAttr` member is stored for introspection.

### Task 2: Add ThreadAttr constructor to FreeRTOS jthread

**read_first:**
- `port_freertos/paraos_jthread.hpp`
- `port_freertos/paraos_thread.hpp`
- `paraos_thread_common.hpp`

**acceptance_criteria:**
1. `port_freertos/paraos_jthread.hpp` defines a new public constructor `jthread(const ThreadAttr&, Function&&, Args&&...)`.
2. The old constructor delegates to the new one with `ThreadAttr{}`.
3. `xTaskCreate` uses `attr.thread_name.data()`, `ConvertStackSizeInWords(attr.stack_depth)` and `static_cast<UBaseType_t>(attr.priority)`.
4. Stored `ThreadAttr` is kept for introspection.

### Task 3: Compile-check all ports

**read_first:**
- `CMakePresets.json`

**acceptance_criteria:**
1. PC smoke test with `ThreadAttr` compiles and runs under `/usr/bin/clang++ -std=c++20`.
2. FreeRTOS smoke test with `ThreadAttr` compiles under `/usr/bin/g++ -std=gnu++17` with FreeRTOS includes.
3. `freertos_debug_gcc` and `pc_debug_clang` presets configure and build successfully.

## Verification

- Smoke tests demonstrate priority/name/stack attributes are accepted at compile time.
- Existing presets build without regressions.

## Risks

- Constructor overload ambiguity between `jthread(Function, Args...)` and `jthread(const ThreadAttr&, Function, Args...)` — resolved by overload preference for `const ThreadAttr&`.
- `std::string_view::data()` for task name must remain valid during `xTaskCreate`; FreeRTOS copies the name internally.
- Setting real-time priority on Unix requires root; failure is silently tolerated.
