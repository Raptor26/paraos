# Design: paraos::jthread scheduler control

## Overview

Add cross-platform scheduler control API to `paraos::jthread` so that user
code can start and stop the threading environment the same way on FreeRTOS,
Linux, and Windows.

- `paraos::jthread::start_scheduler()`
- `paraos::jthread::end_scheduler()`
- `paraos::jthread::is_scheduler_running()`

On FreeRTOS the wrappers delegate to `vTaskStartScheduler()` and
`vTaskEndScheduler()`. On PC the wrappers emulate the FreeRTOS behaviour:
threads created before `start_scheduler()` do not run until the scheduler is
started, and `end_scheduler()` stops all running threads and returns control
to `main()`.

This unifies tests and removes the need for `std::_Exit()` and
`#if defined(PARAOS_LIKE_FREERTOS)` branches in `test_jthread_basic.cpp`.
It also prepares the codebase for the future removal of `paraos::Thread`,
because the scheduler logic will live entirely inside `paraos::jthread`.

## Background

`paraos::Thread` already provides `StartScheduler()` / `Exit()` / `DeleteAll()`
with the desired semantics for FreeRTOS and PC. `paraos::jthread` does not.
As a result, `port_tests/test_jthread_basic.cpp` contains platform-conditional
logic and calls `std::_Exit()` under FreeRTOS because
`vTaskStartScheduler()` never returns.

The project roadmap plans to eventually remove `paraos::Thread` and use only
`paraos::jthread`. Therefore the scheduler control must be implemented inside
`paraos::jthread` without depending on `paraos::Thread`.

## Goals

1. Provide a single, cross-platform API for starting and stopping the
   scheduler from `paraos::jthread`.
2. On FreeRTOS: call `vTaskStartScheduler()` / `vTaskEndScheduler()`.
3. On PC: emulate FreeRTOS semantics:
   - no thread runs before `start_scheduler()`;
   - `end_scheduler()` requests stop, joins all threads, and returns.
4. Rewrite `port_tests/test_jthread_basic.cpp` without `std::_Exit()` and
   without platform-conditional branches, if possible.
5. Keep `paraos::Thread` unchanged; do not introduce dependencies from
   `paraos::jthread` to `paraos::Thread`.
6. Update or verify container multithread tests that already use
   `paraos::jthread` so they start and stop the scheduler correctly.

## Non-goals

- Remove `paraos::Thread` in this milestone.
- Change any existing `paraos::Thread` API.
- Introduce dynamic memory allocation in core scheduler logic beyond what
  `paraos::jthread` already uses for the callable invoker.

## API

```cpp
namespace paraos {
class jthread {
 public:
  /// @brief Start the scheduler.
  ///
  /// FreeRTOS: calls vTaskStartScheduler(). Usually does not return.
  /// PC: releases all jthreads waiting for the scheduler to start.
  static void start_scheduler();

  /// @brief Stop the scheduler.
  ///
  /// FreeRTOS: calls vTaskEndScheduler(). Control returns to the task that
  /// called vTaskStartScheduler() only on ports that implement it.
  /// PC: requests stop on all running jthreads, joins them, and returns.
  static void end_scheduler();

  /// @brief Return whether the scheduler has been started and not ended.
  [[nodiscard]] static auto is_scheduler_running() noexcept -> bool;
};
}  // namespace paraos
```

In addition to the scheduler API, the `paraos::jthread` usage model should
follow the same pattern as `std::jthread` together with
`std::this_thread::sleep_for()`. For example:

```cpp
paraos::jthread worker([](paraos::stop_token token) {
  while (!token.stop_requested()) {
    // Do work.
    paraos::sleep_for(std::chrono::seconds(2));
  }
});

paraos::jthread::start_scheduler();
```

The existing `paraos::sleep_for(std::chrono::milliseconds)` helper is used for
thread sleeps in tests instead of `paraos::Thread::DelayMs()`. This makes the
test code look closer to standard C++20 `std::jthread` examples and avoids
relying on `paraos::Thread` in new `jthread`-based tests.

## FreeRTOS implementation

- `start_scheduler()` → `vTaskStartScheduler()`.
- `end_scheduler()` → `vTaskEndScheduler()`.
- `is_scheduler_running()` → `xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED`.

FreeRTOS itself guarantees that tasks created with `xTaskCreate()` do not run
until the scheduler starts, so no extra gating is required inside
`paraos::jthread`.

The callable invoker and `stop_token` mechanism stay unchanged.

## PC implementation

All PC ports (`port_unix`, `port_win`) include `port_pc/paraos_jthread.hpp`,
so the implementation is added there.

### Scheduler state

A file-scope scheduler state object is added:

```cpp
struct SchedulerState {
  std::atomic<bool> started{false};
  std::atomic<bool> ended{false};
  std::mutex mutex;
  std::condition_variable cv;
  std::vector<Context*> registry;
};
static inline SchedulerState g_scheduler;
```

`Context` is extended with:

- `std::size_t registry_id` — used for safe removal from the registry.
- registration in `g_scheduler.registry` from the constructor;
- deregistration in the destructor.

### Thread gating

Each `std::jthread` callable first waits on `g_scheduler.cv` until either
`started` or `ended` becomes true. If `ended` is true before the thread ever
runs, the callable returns immediately and signals the join semaphore.

```cpp
void WaitForSchedulerStart() {
  std::unique_lock lock{g_scheduler.mutex};
  g_scheduler.cv.wait(lock, [] {
    return g_scheduler.started.load() || g_scheduler.ended.load();
  });
}
```

### start_scheduler()

```cpp
void jthread::start_scheduler() {
  std::lock_guard lock{g_scheduler.mutex};
  g_scheduler.started.store(true);
  g_scheduler.cv.notify_all();
}
```

### end_scheduler()

```cpp
void jthread::end_scheduler() {
  std::vector<Context*> to_stop;
  {
    std::lock_guard lock{g_scheduler.mutex};
    g_scheduler.ended.store(true);
    g_scheduler.cv.notify_all();
    to_stop = g_scheduler.registry;
  }

  for (Context* ctx : to_stop) {
    if (ctx != nullptr && ctx->thread.joinable()) {
      ctx->thread.request_stop();
    }
  }

  for (Context* ctx : to_stop) {
    if (ctx != nullptr && ctx->thread.joinable()) {
      ctx->thread.join();
    }
  }
}
```

`Context` stores a reference to the owning `std::jthread` so that
`end_scheduler()` can request stop and join it. Because `std::jthread` is
movable, the `Context` must hold a pointer to the `jthread` object, not a
reference. The `jthread` object guarantees that `Context` is destroyed before
`std::jthread`, so the pointer is valid for the lifetime of the thread.

### is_scheduler_running()

```cpp
auto jthread::is_scheduler_running() noexcept -> bool {
  return g_scheduler.started.load() && !g_scheduler.ended.load();
}
```

### Join idempotency

`RunTask` always gives the join semaphore, even if it returns early because
`ended` was set. This makes `join()` safe to call after `end_scheduler()`.

## Test changes

### port_tests/test_jthread_basic.cpp

Rewrite the test in a single cross-platform form:

```cpp
auto main() -> int {
  std::size_t size{0};

  paraos::jthread thread([&size](const paraos::stop_token& token) {
    counter.fetch_add(1);
    while (!token.stop_requested()) {
      ++size;
      paraos::sleep_for(std::chrono::milliseconds{10});
    }
  });

  paraos::jthread thread2([](const paraos::stop_token& token) {
    my_func(42, 3.14f);
    while (!token.stop_requested()) {
      paraos::sleep_for(std::chrono::milliseconds{10});
    }
  });

  paraos::jthread stopper([](paraos::stop_token /*token*/) {
    while (counter.load() < kExpectedCounter) {
      paraos::sleep_for(std::chrono::milliseconds{10});
    }
    paraos::jthread::end_scheduler();
  });

  paraos::jthread::start_scheduler();

  // On PC start_scheduler() returns; on FreeRTOS it does not return.
#if !defined(PARAOS_LIKE_FREERTOS)
  if (counter.load() == kExpectedCounter) {
    std::cout << "OK\n";
    return EXIT_SUCCESS;
  }
  std::cout << "FAIL: counter=" << counter.load() << "\n";
  return EXIT_FAILURE;
#endif
}
```

If FreeRTOS POSIX/Windows port does not correctly terminate the process after
`vTaskEndScheduler()`, the stopper task may fall back to `std::_Exit()` as a
last resort. The goal is to avoid this fallback, but it is documented here as
an acceptable escape hatch.

### Container multithread tests

Tests in `containers/tests/` that use `paraos::jthread` should be inspected.
Most create threads and rely on scope-bound destruction. They may need an
explicit `paraos::jthread::start_scheduler()` before threads are expected to
run. Because on PC threads currently start immediately, adding `start_scheduler()`
after construction is safe and aligns the tests with FreeRTOS semantics.

## Risks and fallback

| Risk | Mitigation |
|------|------------|
| `vTaskEndScheduler()` on FreeRTOS POSIX/Windows port does not return to main | Keep `std::_Exit()` as a documented fallback inside the stopper task only if needed. |
| `end_scheduler()` on PC deadlocks if called from a thread while holding state | Do not hold `g_scheduler.mutex` while joining threads. |
| `Context*` becomes dangling after `jthread` destruction | Deregister in `Context` destructor and never access `Context` after `end_scheduler()` returns. |
| Clang-tidy warnings about global mutable state | Encapsulate state in `SchedulerState`; use `static inline` inside header. |

## Files to modify

- `port_freertos/paraos_jthread.hpp` — add scheduler API.
- `port_pc/paraos_jthread.hpp` — add scheduler state, gating, and API.
- `port_tests/test_jthread_basic.cpp` — remove platform branches and `std::_Exit()`.
- `containers/tests/test_multi_ringbuff_mpmc.cpp` — add `start_scheduler()` if needed.
- `containers/tests/test_message_multithread_many_producer_many_consumers.cpp` — add `start_scheduler()` if needed.
- `containers/tests/test_queue_blocking_mpsc.cpp` — add `start_scheduler()` if needed.
- `containers/tests/test_queue_blocking_spmc.cpp` — add `start_scheduler()` if needed.
- `containers/tests/test_queue_blocking_mpmc.cpp` — add `start_scheduler()` if needed.
- `port_tests/CMakeLists.txt` — verify CTest registration; no expected changes.

## Success criteria

1. `pc_debug_clang` and `pc_debug_gcc` presets build and pass `ctest`.
2. `*_clang_tidy` presets build without new warnings.
3. `freertos_debug_clang` and `freertos_debug_gcc` presets compile.
4. `test_jthread_basic.cpp` no longer contains `std::_Exit()` or
   platform-conditional branches in the test logic. A single `#if !defined(PARAOS_LIKE_FREERTOS)` guard around code that runs after `start_scheduler()` is acceptable, because `vTaskStartScheduler()` does not return on FreeRTOS.
5. New and updated tests use `paraos::sleep_for(std::chrono::milliseconds{...})`
   instead of `paraos::Thread::DelayMs()` to look like
   `std::this_thread::sleep_for(std::chrono::seconds(2))` usage.
