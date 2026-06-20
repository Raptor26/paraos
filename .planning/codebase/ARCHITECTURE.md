# Architecture

**Analysis Date:** 2026-06-20

## Pattern Overview

**Overall:** Cross-platform Operating System Abstraction Layer (OSAL) delivered as a source-only static CMake library with pluggable port implementations.

**Key Characteristics:**
- Header-only core with platform-specific headers under `port_*` directories.
- Port selected at configure time via CMake (`RTOS_NAME=FREERTOS`, `WIN32`, `UNIX`).
- Common C++ API surface regardless of backend (POSIX, WinAPI, FreeRTOS).
- RAII wrappers for all OS primitives.
- Heavy use of ETL delegates and containers to avoid dynamic allocation on embedded targets.
- Static library target `paraos` with alias `paraos::paraos`; compile definitions and include paths propagated through interface target `paraos_setup`.

## Layers

**Core Abstraction Layer (root `*.hpp` / `*.h`):**
- Purpose: Define the portable public API shared by all ports.
- Contains: `paraos_thread_common.hpp`, `paraos_mutex_raii.hpp`, `paraos_bool_atomic.hpp`, `paraos_runtime_profiler.hpp`, `paraos_isr.hpp`, `paraos_exceptions.hpp`, `paraos_check.h`, `paraos_trace.hpp`, etc.
- Depends on: ETL, GSL, port-specific headers for primitive implementations (`paraos_mutex.hpp`, `paraos_thread.hpp`, etc.).
- Used by: Application code and higher-level helpers (`extra/`).

**Port Layer (`port_unix/`, `port_win/`, `port_freertos/`):**
- Purpose: Backend implementation of OS primitives for each supported platform.
- Contains: `paraos_thread.hpp`, `paraos_mutex.hpp`, `paraos_semaphore.hpp`, `paraos_timer.hpp`, `paraos_critical.hpp`, `paraos_time.hpp`, `paraos_utils.hpp`, plus port-specific `paraos_socket_udp.hpp` where applicable.
- Depends on: Platform APIs (`pthread`, WinAPI, FreeRTOS), core headers.
- Used by: Core and application code through a single API.

**Container Layer (`containers/`):**
- Purpose: Thread-safe message buffers, blocking queues, ring buffers, and multi-ring buffers.
- Contains: `paraos_message_buffer.hpp`, `paraos_queue_blocking.hpp`, `paraos_ringbuff.hpp`, `paraos_multi_ringbuff.hpp`.
- Depends on: Core mutex/semaphore/critical-section abstractions, ETL queues, LwRB.
- Used by: Application code and extra helpers.

**Helper Layer (`extra/`):**
- Purpose: Higher-level reusable components built on top of core primitives.
- Contains: `paraos_oneshot_executor.hpp`, `paraos_thread_cooperative_scheduling.hpp`, `paraos_thread_sequence.hpp`, `paraos_status_led.hpp`/`cpp`.
- Depends on: `paraos::Thread`, `paraos::QueueBlocking`, ETL scheduler/timer.
- Used by: Applications needing status LEDs, cooperative schedulers, periodic delegate sequences.

**Test Layer (`port_tests/`, `containers/tests/`, `extra/tests/`, `port_unix/tests/`):**
- Purpose: Unit tests, OSAL smoke tests, stress tests, and standalone examples.
- Contains: GoogleTest-based executables, stress executables labeled `stress`, and non-CTest example binaries (`example_timer`, `example_socket_udp`, etc.).

## Data Flow

**Typical OS Primitive Call:**

1. Application calls `paraos::MutexGuard guard{mutex};` (`paraos_mutex_raii.hpp`).
2. `MutexGuard` invokes `MutexBase::Lock(timeout_ms)` from the active port (`port_unix/paraos_mutex.hpp`, etc.).
3. Port translates the call to the native primitive (`pthread_mutex_lock` / `pthread_mutex_timedlock`, WinAPI, or FreeRTOS semaphore/mutex).
4. On scope exit, `MutexGuard` calls `Unlock()`.

**Thread Lifecycle:**

1. Application constructs `paraos::Thread` with a `paraos::ThreadAttr` containing an `etl::delegate<void()>`.
2. Port-specific `Make()` creates the native thread/task.
3. The thread waits on a binary semaphore until `paraos::Thread::StartScheduler()` is called (Unix/FreeRTOS patterns differ).
4. The delegate is invoked repeatedly in a loop until `Finished()` is called.
5. `Finished()` can optionally schedule deferred deletion of a heap-allocated `paraos::Base` subclass.

**Cooperative Scheduler Flow (`extra/paraos_thread_cooperative_scheduling.hpp`):**

1. `CooperativeScheduling` inherits from both `etl::scheduler` and `ICooperativeScheduling`.
2. `ICooperativeScheduling` owns a `paraos::Thread` and registers `Run()` as the thread delegate.
3. `Run()` calls `scheduler_.start()`, which processes registered `etl::task` instances.
4. When no tasks have work, the scheduler calls `Idle()`, which waits on `new_cycle_ready_sem_`.
5. `NotifyGive()` (optionally from ISR) releases the semaphore and starts the next cycle.

## Key Abstractions

**`paraos::Base` (`paraos_base.hpp`):**
- Purpose: Optional deferred-deletion callback base class.
- Pattern: Abstract base with virtual destructor invoking a user callback.

**`paraos::Thread` (`port_*/paraos_thread.hpp`):**
- Purpose: Cross-platform thread/task abstraction.
- Pattern: Non-copyable, non-movable, owns a native handle and an `etl::delegate<void()>`.

**`paraos::MutexBase` / `Mutex` / `MutexRecursive` (`port_*/paraos_mutex.hpp`):**
- Purpose: Cross-platform mutex abstraction.
- Pattern: Base class with virtual destructor; concrete classes set recursive vs normal behavior.

**`paraos::CriticalSection` (`port_*/paraos_critical.hpp`):**
- Purpose: RAII wrapper around a global recursive mutex to serialize access.
- Pattern: Static mutex + template parameter to control ISR capability.

**`paraos::ISRbool` (`paraos_isr.hpp`):**
- Purpose: Carry both operation success and context-switch request from ISR calls.
- Pattern: Small value type used by semaphore/mutex/queue ISR APIs.

**`paraos::IQueueBlocking` (`containers/paraos_queue_blocking.hpp`):**
- Purpose: Blocking producer/consumer queue.
- Pattern: Interface + semaphore-based synchronization, supports ISR-aware push/pop.

## Entry Points

**Library Entry:**
- `CMakeLists.txt` — Selects port, wires dependencies, creates `paraos` target.
- `setup.cmake` — Creates `paraos_setup` interface target.

**Application Entry:**
- Consumer includes root headers and links `paraos::paraos`.
- No runtime `main()` in the library; tests provide their own `main()` through `GTest::gtest_main`.

**Build/CI Entry:**
- `builder.py` — Interactive / non-interactive build and test runner.
- `pybuilder/pycmakebuilder.py` — CI driver used by `.gitlab-ci.yml`.

## Error Handling

**Strategy:** Exception-based for PC builds; ETL error macros (`ETL_ASSERT`) also used for embedded-friendly failures.

**Patterns:**
- `paraos::exception` derives from both `std::exception` and `etl::exception` (`paraos_exceptions.hpp`).
- Custom exceptions for thread creation (`thread_not_created_exception`) and thread sequence (`InvalidThreadSequenceException`).
- `PARAOS_CHECK_ASSERT(x)` / `PARAOS_CHECK_LOOP()` expand to `assert(x)` or infinite loop depending on `PARAOS_CHECK_LOOP_ENABLE`.
- ETL asserts throw `etl::exception`-based types.

## Cross-Cutting Concerns

**Tracing:**
- `paraosTRACE_MESSAGE`, `paraosTRACE_MESSAGE_WITH_ACTOR_NAME`, `paraosOUT` macros.
- Enabled by `paraosTRACE_ENABLE`; prints to `std::cout` under a critical section.

**ISR Safety:**
- `is_isr` parameter threaded through mutex/semaphore/queue APIs.
- FreeRTOS port uses ISR variants (`xSemaphoreGiveFromISR`, etc.).
- `DisableIsr()` / `EnableIsr()` helpers on capable ports.

**Portability Macros:**
- `PARAOS_LIKE_UNIX`, `PARAOS_LIKE_WINAPI`, `PARAOS_LIKE_FREERTOS`.
- `PARAOS_ATTR_*` macros abstract GCC/Clang/MSVC attributes (`paraos_attr.h`).
- `PARAOS_POLYMORPHIC_EXTRA` switches `virtual` on helper methods for mockability.

---

*Architecture analysis: 2026-06-20*
*Update when major patterns or port structure changes*
