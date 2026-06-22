# paraos::jthread scheduler control — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add cross-platform `start_scheduler()` / `end_scheduler()` / `is_scheduler_running()` to `paraos::jthread`, emulate FreeRTOS scheduler semantics on PC, and unify `jthread`-based tests so they no longer need `std::_Exit()` or platform-conditional test logic.

**Architecture:** Static scheduler control methods are added to `paraos::jthread`. FreeRTOS delegates to `vTaskStartScheduler()`/`vTaskEndScheduler()`. PC ports keep a file-scope scheduler state with a condition variable to gate thread startup and a registry of active contexts so `end_scheduler()` can stop and join all threads. Tests are updated to call `start_scheduler()` after constructing `jthread`s and to use `paraos::sleep_for()` instead of `paraos::Thread::DelayMs()`.

**Tech Stack:** C++20, FreeRTOS-Kernel POSIX/Windows ports, `std::jthread`, `std::condition_variable`, `std::atomic`, `std::mutex`, `std::vector`, ETL atomic/semaphore, GoogleTest/CTest, CMake, clang-tidy.

## Global Constraints

- All public symbols live in namespace `paraos`.
- C++ minimum is C++20 for `paraos::jthread` consumers; core library already requires C++20.
- All compiler warnings are errors (`-Wall -Wextra -Wpedantic -Werror`).
- clang-tidy treats warnings as errors (`WarningsAsErrors: '*'`).
- No new dependencies from `paraos::jthread` to `paraos::Thread`.
- `paraos::Thread` API must remain unchanged.
- PC implementation must be in `port_pc/paraos_jthread.hpp` so it is shared by `port_unix` and `port_win`.
- FreeRTOS implementation is in `port_freertos/paraos_jthread.hpp`.
- Tests must build and pass on `pc_debug_clang`, `pc_debug_gcc`, and compile on `freertos_debug_clang`/`freertos_debug_gcc`.
- Commit messages are in Russian, Commitizen style, short passive past participles.

---

## File map

| File | Responsibility |
|------|----------------|
| `port_freertos/paraos_jthread.hpp` | FreeRTOS `jthread` implementation. Add scheduler API. |
| `port_pc/paraos_jthread.hpp` | PC `jthread` implementation. Add scheduler state, gating, registry, and API. |
| `port_tests/test_jthread_basic.cpp` | Rewrite without platform-conditional test logic and without `std::_Exit()`. |
| `containers/tests/test_multi_ringbuff_mpmc.cpp` | Add `start_scheduler()` and remove `std::_Exit()` if possible. |
| `containers/tests/test_message_multithread_many_producer_many_consumers.cpp` | Add `start_scheduler()` and remove `std::_Exit()` if possible. |
| `containers/tests/test_queue_blocking_mpsc.cpp` | Add `start_scheduler()` and remove `std::_Exit()` if possible. |
| `containers/tests/test_queue_blocking_spmc.cpp` | Add `start_scheduler()` and remove `std::_Exit()` if possible. |
| `containers/tests/test_queue_blocking_mpmc.cpp` | Add `start_scheduler()` and remove `std::_Exit()` if possible. |

---

## Task 1: Add scheduler API to FreeRTOS `paraos::jthread`

**Files:**
- Modify: `port_freertos/paraos_jthread.hpp`

**Interfaces:**
- Produces: `static void paraos::jthread::start_scheduler();`
- Produces: `static void paraos::jthread::end_scheduler();`
- Produces: `[[nodiscard]] static auto paraos::jthread::is_scheduler_running() noexcept -> bool;`

- [ ] **Step 1: Add public static methods to `jthread`**

In `port_freertos/paraos_jthread.hpp`, in the `jthread` class public section after `joinable()`, add:

```cpp
  /// @brief Start the FreeRTOS scheduler.
  ///
  /// Calls vTaskStartScheduler(). On most ports this function does not
  /// return.
  static void start_scheduler() { vTaskStartScheduler(); }

  /// @brief Stop the FreeRTOS scheduler.
  ///
  /// Calls vTaskEndScheduler(). The calling task is responsible for deleting
  /// itself afterwards if the port returns from this function.
  static void end_scheduler() { vTaskEndScheduler(); }

  /// @brief Return whether the scheduler has been started.
  [[nodiscard]] static auto is_scheduler_running() noexcept -> bool {
    return xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED;
  }
```

- [ ] **Step 2: Verify FreeRTOS build compiles**

Run:
```bash
cmake --preset freertos_debug_clang
cmake --build build/freertos_debug_clang/ --target test_jthread_basic
```

Expected: build succeeds, possibly link/runtime skipped on macOS because POSIX timer APIs are unavailable.

- [ ] **Step 3: Commit**

```bash
git add port_freertos/paraos_jthread.hpp
git commit -m "feat(jthread): добавлены методы управления планировщиком для FreeRTOS"
```

---

## Task 2: Add scheduler state and gating to PC `paraos::jthread`

**Files:**
- Modify: `port_pc/paraos_jthread.hpp`

**Interfaces:**
- Consumes: existing `Context` struct.
- Produces: file-scope `SchedulerState g_scheduler`.
- Produces: `static void paraos::jthread::start_scheduler();`
- Produces: `static void paraos::jthread::end_scheduler();`
- Produces: `[[nodiscard]] static auto paraos::jthread::is_scheduler_running() noexcept -> bool;`

- [ ] **Step 1: Add required standard headers**

Add near the top of `port_pc/paraos_jthread.hpp` after existing includes:

```cpp
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>
```

- [ ] **Step 2: Add scheduler state and `Context` type inside `jthread` private section**

Inside class `jthread`, in this order in the private section, add:

```cpp
 private:
  struct Context;

  struct SchedulerState {
    std::atomic<bool> started{false};
    std::atomic<bool> ended{false};
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<Context*> registry;
  };

  static inline SchedulerState g_scheduler;

  struct Context {
    etl::atomic_bool stop_flag{false};
    InvokerBase* invoker{nullptr};
    SemaphoreBinary join_sem;
    std::jthread* thread{nullptr};

    explicit Context(InvokerBase* invoker_in) : invoker(invoker_in) {
      std::lock_guard<std::mutex> lock{g_scheduler.mutex};
      g_scheduler.registry.push_back(this);
    }

    ~Context() {
      {
        std::lock_guard<std::mutex> lock{g_scheduler.mutex};
        auto it = std::find(g_scheduler.registry.begin(),
                            g_scheduler.registry.end(), this);
        if (it != g_scheduler.registry.end()) {
          g_scheduler.registry.erase(it);
        }
      }
      delete invoker;
    }
  };
```

- [ ] **Step 3: Add required includes for PC port**

Add near the top of `port_pc/paraos_jthread.hpp` after existing includes:

```cpp
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "paraos_semaphore.hpp"
```

- [ ] **Step 4: Update `MakeThread` to allocate `Context` and gate thread startup**

Replace the existing PC `MakeThread` with:

```cpp
  template <typename Function, typename... Args>
  void MakeThread(const ThreadAttr& attr, Function&& func, Args&&... args) {
    using decayed_function = std::decay_t<Function>;
    using decayed_args = std::tuple<std::decay_t<Args>...>;

    auto* invoker = new Invoker<decayed_function, decayed_args>(
        std::forward<Function>(func), std::forward<Args>(args)...);

    context_ = new Context{invoker};
    attr_ = attr;

    context_->thread = &thread_;
    thread_ = std::jthread(
        [ctx = context_](std::stop_token std_token) mutable -> void {
          WaitForSchedulerStart();
          if (g_scheduler.ended.load()) {
            ctx->join_sem.Give();
            return;
          }
          if (ctx->invoker != nullptr) {
            ctx->invoker->Invoke(stop_token{std::move(std_token)});
          }
          ctx->join_sem.Give();
        });
    ApplyAttr();
  }
```

- [ ] **Step 5: Add `WaitForSchedulerStart` static helper**

In private section, add before `MakeThread`:

```cpp
  static void WaitForSchedulerStart() {
    std::unique_lock<std::mutex> lock{g_scheduler.mutex};
    g_scheduler.cv.wait(lock, [] {
      return g_scheduler.started.load() || g_scheduler.ended.load();
    });
  }
```

- [ ] **Step 6: Update destructor and move operations to use `context_`**

Old destructor:
```cpp
  ~jthread() {
    if (thread_.joinable()) {
      thread_.request_stop();
      thread_.join();
    }
  }
```

New destructor:
```cpp
  ~jthread() {
    if (joinable()) {
      request_stop();
      join();
    }
    delete context_;
  }
```

Move constructor old:
```cpp
  jthread(jthread&& other) noexcept = default;
```

New move constructor:
```cpp
  jthread(jthread&& other) noexcept
      : context_{other.context_}, attr_{other.attr_}, thread_{std::move(other.thread_)} {
    other.context_ = nullptr;
    if (context_ != nullptr) {
      context_->thread = &thread_;
    }
  }
```

Move assignment old:
```cpp
  auto operator=(jthread&& other) noexcept -> jthread& = default;
```

New move assignment:
```cpp
  auto operator=(jthread&& other) noexcept -> jthread& {
    if (this != &other) {
      if (joinable()) {
        request_stop();
        join();
      }
      delete context_;
      context_ = other.context_;
      attr_ = other.attr_;
      thread_ = std::move(other.thread_);
      other.context_ = nullptr;
      if (context_ != nullptr) {
        context_->thread = &thread_;
      }
    }
    return *this;
  }
```

- [ ] **Step 7: Update `request_stop`, `join`, `joinable`**

Old:
```cpp
  [[nodiscard]] auto request_stop() noexcept -> bool {
    return thread_.request_stop();
  }

  void join() { thread_.join(); }

  [[nodiscard]] auto joinable() const noexcept -> bool {
    return thread_.joinable();
  }
```

New:
```cpp
  [[nodiscard]] auto request_stop() noexcept -> bool {
    if (context_ != nullptr) {
      context_->stop_flag.store(true);
      if (thread_.joinable()) {
        thread_.request_stop();
      }
      return true;
    }
    return false;
  }

  void join() {
    if (context_ != nullptr) {
      context_->join_sem.Take();
    }
  }

  [[nodiscard]] auto joinable() const noexcept -> bool {
    return context_ != nullptr && thread_.joinable();
  }
```

- [ ] **Step 8: Add public scheduler API methods**

In public section add:

```cpp
  /// @brief Start the scheduler.
  ///
  /// On PC this releases all jthreads waiting for the scheduler to start.
  static void start_scheduler() {
    std::lock_guard<std::mutex> lock{g_scheduler.mutex};
    g_scheduler.started.store(true);
    g_scheduler.cv.notify_all();
  }

  /// @brief Stop the scheduler.
  ///
  /// On PC this requests stop on all running jthreads, joins them, and
  /// returns.
  static void end_scheduler() {
    std::vector<Context*> to_stop;
    {
      std::lock_guard<std::mutex> lock{g_scheduler.mutex};
      g_scheduler.ended.store(true);
      g_scheduler.cv.notify_all();
      to_stop = g_scheduler.registry;
    }

    for (Context* ctx : to_stop) {
      if (ctx != nullptr && ctx->thread != nullptr &&
          ctx->thread->joinable()) {
        ctx->stop_flag.store(true);
        ctx->thread->request_stop();
      }
    }

    for (Context* ctx : to_stop) {
      if (ctx != nullptr && ctx->thread != nullptr &&
          ctx->thread->joinable()) {
        ctx->thread->join();
      }
    }
  }

  /// @brief Return whether the scheduler has been started and not ended.
  [[nodiscard]] static auto is_scheduler_running() noexcept -> bool {
    return g_scheduler.started.load() && !g_scheduler.ended.load();
  }
```

- [ ] **Step 9: Build PC preset and run `test_jthread_basic`**

Run:
```bash
cmake --preset pc_debug_clang
cmake --build build/pc_debug_clang/ --target test_jthread_basic
ctest --test-dir build/pc_debug_clang/ -R test_jthread_basic --output-on-failure
```

Expected: `test_jthread_basic` currently still uses old test logic with `paraos::Thread::StartScheduler()`, so it may hang or fail. We will update the test in Task 3. For now only verify compilation.

Expected compilation result: success.

- [ ] **Step 11: Commit**

```bash
git add port_pc/paraos_jthread.hpp
git commit -m "feat(jthread): добавлено управление планировщиком для PC портов"
```

---

## Task 3: Rewrite `test_jthread_basic.cpp`

**Files:**
- Modify: `port_tests/test_jthread_basic.cpp`

**Interfaces:**
- Consumes: `paraos::jthread::start_scheduler()`
- Consumes: `paraos::jthread::end_scheduler()`
- Consumes: `paraos::sleep_for(std::chrono::milliseconds)`

- [ ] **Step 1: Update includes**

Old includes:
```cpp
#include <atomic>
#include <cstdlib>
#include <iostream>

#include "paraos_jthread.hpp"
#include "paraos_thread.hpp"
```

New includes:
```cpp
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>

#include "paraos_jthread.hpp"
#include "paraos_sleep.hpp"
```

- [ ] **Step 2: Rewrite `main()`**

Old `main()`:
```cpp
auto main() -> int {
  {
    std::size_t size{0};
    paraos::jthread thread([&size](const paraos::stop_token& token) {
      counter.fetch_add(1);
      while (!token.stop_requested()) {
        ++size;
        paraos::Thread::DelayMs(10);
      }
    });

    paraos::jthread thread2([](const paraos::stop_token& token) {
      my_func(42, 3.14f);
      while (!token.stop_requested()) {
        paraos::Thread::DelayMs(10);
      }
    });

#if defined(PARAOS_LIKE_FREERTOS)
    paraos::jthread stopper([&thread, &thread2](paraos::stop_token /*token*/) {
      while (counter.load() < 2) {
        paraos::Thread::DelayMs(10);
      }

      (void)thread.request_stop();
      thread.join();
      (void)thread2.request_stop();
      thread2.join();

      std::_Exit(
          counter.load() == kExpectedCounter ? EXIT_SUCCESS : EXIT_FAILURE);
    });
#endif

    paraos::Thread::StartScheduler();

#if !defined(PARAOS_LIKE_FREERTOS)
    while (counter.load() < 2) {
      paraos::Thread::DelayMs(10);
    }

    (void)thread.request_stop();
    thread.join();
    (void)thread2.request_stop();
    thread2.join();
#endif
  }

#if !defined(PARAOS_LIKE_FREERTOS)
  if (counter.load() == kExpectedCounter) {
    std::cout << "OK\n";
  } else {
    std::cout << "FAIL: counter=" << counter.load() << "\n";
    return EXIT_FAILURE;
  }

  paraos::Thread::Exit();
#endif

  return 0;
}
```

New `main()`:
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

#if !defined(PARAOS_LIKE_FREERTOS)
  // On FreeRTOS vTaskStartScheduler() does not return.
  if (counter.load() == kExpectedCounter) {
    std::cout << "OK\n";
    return EXIT_SUCCESS;
  }

  std::cout << "FAIL: counter=" << counter.load() << "\n";
  return EXIT_FAILURE;
#endif
}
```

- [ ] **Step 3: Build and run on PC**

Run:
```bash
cmake --build build/pc_debug_clang/ --target test_jthread_basic
ctest --test-dir build/pc_debug_clang/ -R test_jthread_basic --output-on-failure
```

Expected: `test_jthread_basic` passes.

- [ ] **Step 4: Run on PC GCC preset**

Run:
```bash
cmake --preset pc_debug_gcc
cmake --build build/pc_debug_gcc/ --target test_jthread_basic
ctest --test-dir build/pc_debug_gcc/ -R test_jthread_basic --output-on-failure
```

Expected: passes.

- [ ] **Step 5: Commit**

```bash
git add port_tests/test_jthread_basic.cpp
git commit -m "refactor(tests): переписан test_jthread_basic на единый кроссплатформенный вид"
```

---

## Task 4: Update container multithread tests to use `start_scheduler()` and remove `std::_Exit()`

**Files:**
- Modify: `containers/tests/test_multi_ringbuff_mpmc.cpp`
- Modify: `containers/tests/test_message_multithread_many_producer_many_consumers.cpp`
- Modify: `containers/tests/test_queue_blocking_mpsc.cpp`
- Modify: `containers/tests/test_queue_blocking_spmc.cpp`
- Modify: `containers/tests/test_queue_blocking_mpmc.cpp`

**Interfaces:**
- Consumes: `paraos::jthread::start_scheduler()`
- Consumes: `paraos::jthread::end_scheduler()` (optional, via scope destruction)

- [ ] **Step 1: Update `test_queue_blocking_mpsc.cpp`**

Locate:
```cpp
  {
    std::vector<paraos::jthread> threads;
    ...
  }

  CheckIfTestSuccessfullyComplete();

#ifdef PARAOS_LIKE_FREERTOS
  std::_Exit(EXIT_SUCCESS);
#else
  return 0;
#endif
```

Change to:
```cpp
  {
    std::vector<paraos::jthread> threads;
    ...

    paraos::jthread::start_scheduler();
  }

  CheckIfTestSuccessfullyComplete();

  return 0;
```

Remove the `#ifdef PARAOS_LIKE_FREERTOS ... #endif` block around return.

- [ ] **Step 2: Repeat for `test_queue_blocking_spmc.cpp`, `test_queue_blocking_mpmc.cpp`, `test_multi_ringbuff_mpmc.cpp`, `test_message_multithread_many_producer_many_consumers.cpp`**

For each file:
1. Find the inner scope with `std::vector<paraos::jthread> threads;`.
2. Add `paraos::jthread::start_scheduler();` at the end of that scope after all threads are constructed.
3. Replace the final `#ifdef PARAOS_LIKE_FREERTOS std::_Exit(EXIT_SUCCESS); #else return 0; #endif` with plain `return 0;`.

- [ ] **Step 3: Build and run container tests on PC**

Run:
```bash
cmake --build build/pc_debug_clang/
ctest --test-dir build/pc_debug_clang/ -L stress --output-on-failure --timeout 30
```

Expected: container stress tests pass.

- [ ] **Step 4: Commit**

```bash
git add containers/tests/test_multi_ringbuff_mpmc.cpp \
        containers/tests/test_message_multithread_many_producer_many_consumers.cpp \
        containers/tests/test_queue_blocking_mpsc.cpp \
        containers/tests/test_queue_blocking_spmc.cpp \
        containers/tests/test_queue_blocking_mpmc.cpp
git commit -m "refactor(tests): добавлен start_scheduler() и удален std::_Exit() в контейнерных тестах"
```

---

## Task 5: clang-tidy and full PC verification

**Files:**
- Modify: none, only verification.

- [ ] **Step 1: Run clang-tidy preset**

Run:
```bash
cmake --preset pc_debug_gcc_clang_tidy
cmake --build build/pc_debug_gcc_clang_tidy/ --target test_jthread_basic
ctest --test-dir build/pc_debug_gcc_clang_tidy/ -R test_jthread_basic --output-on-failure
```

Expected: no clang-tidy warnings treated as errors; test passes.

- [ ] **Step 2: Run full PC CTest**

Run:
```bash
ctest --test-dir build/pc_debug_clang/ --output-on-failure --stop-on-failure --schedule-random --timeout 30
ctest --test-dir build/pc_debug_gcc/ --output-on-failure --stop-on-failure --schedule-random --timeout 30
```

Expected: all tests pass.

- [ ] **Step 3: Build FreeRTOS presets**

Run:
```bash
cmake --preset freertos_debug_clang
cmake --build build/freertos_debug_clang/ --target test_jthread_basic
cmake --preset freertos_debug_gcc
cmake --build build/freertos_debug_gcc/ --target test_jthread_basic
```

Expected: compile succeeds. Runtime on macOS may fail due to POSIX timer unavailability; this is a known environment limitation, not a regression.

- [ ] **Step 4: Commit any tidy fixes**

If clang-tidy found issues, fix and commit:

```bash
git add <fixed-files>
git commit -m "refactor(jthread): исправлены замечания clang-tidy"
```

---

## Task 6: Documentation update

**Files:**
- Modify: `.planning/PROJECT.md`
- Modify: `docs/superpowers/specs/2026-06-22-jthread-scheduler-design.md` if deviations found.

- [ ] **Step 1: Update `.planning/PROJECT.md` current milestone section**

Add a new shipped milestone entry for this work. Use the current version from `.cz.json`. Example:

```markdown
**Shipped:** v1.6 paraos::jthread scheduler control (2026-06-22)

- Добавлены `paraos::jthread::start_scheduler()`, `paraos::jthread::end_scheduler()`
  и `paraos::jthread::is_scheduler_running()` для FreeRTOS и PC.
- FreeRTOS-порт делегирует вызовы в `vTaskStartScheduler()` / `vTaskEndScheduler()`.
- PC-порт эмулирует поведение FreeRTOS: потоки не запускаются до вызова
  `start_scheduler()`, а `end_scheduler()` останавливает и объединяет все потоки.
- `port_tests/test_jthread_basic.cpp` переписан без `std::_Exit()` и
  платформенных ветвей в тестовой логике.
- Контейнерные multithread-тесты обновлены для явного запуска планировщика.
```

- [ ] **Step 2: Commit**

```bash
git add .planning/PROJECT.md
git commit -m "docs(project): обновлен PROJECT.md по вехе управления планировщиком jthread"
```

---

## Self-review checklist

1. **Spec coverage:**
   - FreeRTOS scheduler wrappers — Task 1.
   - PC scheduler state + gating — Task 2.
   - `start_scheduler()` / `end_scheduler()` / `is_scheduler_running()` — Tasks 1 and 2.
   - Unified `test_jthread_basic.cpp` — Task 3.
   - Container test updates — Task 4.
   - `paraos::sleep_for` usage — Tasks 3 and 4.
   - No `paraos::Thread` dependency introduced — enforced in Task 2 design.

2. **Placeholder scan:** no TBD/TODO/implement-later placeholders.

3. **Type consistency:**
   - `start_scheduler()` and `end_scheduler()` are `static void` in both ports.
   - `is_scheduler_running()` is `[[nodiscard]] static auto ... -> bool noexcept` in both ports.
   - `Context` type is introduced only in PC; FreeRTOS keeps existing `Context`.
   - `paraos::sleep_for(std::chrono::milliseconds{...})` is used consistently.

## Execution handoff

Plan complete and saved to `docs/superpowers/plans/2026-06-22-jthread-scheduler-plan.md`.

Two execution options:

1. **Subagent-Driven (recommended)** — dispatch a fresh subagent per task, review between tasks, fast iteration.
2. **Inline Execution** — execute tasks in this session using `executing-plans`, batch execution with checkpoints.

Which approach?
