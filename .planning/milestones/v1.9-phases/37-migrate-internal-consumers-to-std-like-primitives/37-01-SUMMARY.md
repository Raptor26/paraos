# Phase 37: Migrate internal consumers to std-like primitives — Summary

**Completed:** 2026-06-24
**Status:** Complete

## What Changed

- Introduced `paraos::recursive_mutex` for all ports:
  - `port_pc/paraos_recursive_mutex_std.hpp` wrapping `std::recursive_mutex`.
  - Forwarding headers in `port_unix/` and `port_win/`.
  - `port_freertos/paraos_recursive_mutex.hpp` using `xSemaphoreCreateRecursiveMutex()`.
- Migrated `containers/paraos_queue_blocking.hpp` and `containers/paraos_message_buffer.hpp` to `paraos::mutex` and `paraos::counting_semaphore`.
  - `IQueueBlocking` and `IMessageBuffer` now propagate the compile-time `SIZE` so the std-like semaphore max matches the queue capacity.
- Migrated `containers/paraos_multi_ringbuff.hpp` to use `IQueueBlocking<std::size_t, QUEUE_SIZE>`.
- Migrated `port_unix/paraos_critical.hpp` from legacy `MutexRecursive` to `paraos::recursive_mutex`.
- Migrated `port_unix/paraos_socket_udp.hpp` and `port_win/paraos_socket_udp.hpp` from `paraos::Thread::DelayMs` to `paraos::sleep_for(std::chrono::milliseconds(...))`.
- Migrated `port_freertos/paraos_jthread.hpp` from legacy `SemaphoreBinary` to `paraos::binary_semaphore`.
- Migrated `extra/paraos_thread_sequence.hpp` and `extra/paraos_thread_cooperative_scheduling.hpp` from `SemaphoreBinary` to `paraos::binary_semaphore`.
- Migrated `extra/paraos_oneshot_executor.hpp` to `IQueueBlocking<executor_delegate_type, QUEUE_SIZE>`.
- Added missing `#include "paraos_utils.hpp"` to `port_unix/paraos_time.hpp` so `paraos::delay_type` remains available after the legacy mutex include chain was removed.
- Fixed `ThreadAttr` initializers in `extra/tests/` that still passed the removed `dtor_callback` field.

## Verification

- `pc_debug_clang` preset configures and builds the `paraos` library, `test_paraos_containers`, and `test_paraos_extra` targets.
- `freertos_debug_clang` preset configures and builds the `paraos` library, `test_paraos_containers`, and `test_paraos_extra` targets.

## Notes

- `port_tests/` legacy test/example files are still being compiled by CMake; full `ctest` run is deferred to Phase 39 after Phase 38 removes or rewrites them.
