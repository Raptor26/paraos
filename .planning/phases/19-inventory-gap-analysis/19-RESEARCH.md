# Phase 19 Research: Inventory & Gap Analysis

**Date:** 2026-06-22
**Phase:** 19 — Inventory & gap analysis
**Researcher:** gsd-plan-phase

## Executive Summary

The v1.5 milestone migrates multithreaded container tests from the legacy `paraos::Thread` API to the std-like `paraos::jthread`, `paraos::mutex`, and `paraos::*_semaphore` APIs. This research audits the five multithreaded tests in `containers/tests/`, compares the legacy and std-like primitives at the source level, and identifies the minimum FreeRTOS-specific hardening required for the migrated tests to behave uniformly across PC, Unix, Windows, and FreeRTOS.

Key findings:

- All five multithreaded tests use **only** `paraos::Thread` and `paraos::Thread::DelayMs`. No test uses `paraos::Mutex`, `MutexGuard`, `paraos::SemaphoreBinary`, or `paraos::SemaphoreCounting`.
- The migration therefore reduces to replacing the thread primitive and the delay/yield helper; Phase 21 synchronization work is expected to be minimal.
- The legacy `StartScheduler()` / `DeleteAll()` / `Finished()` / `Exit()` lifecycle has no direct equivalent in `paraos::jthread`. The new pattern is: local `std::vector<paraos::jthread>`, RAII join, final assertions in `main()` after the inner scope, and `std::_Exit(EXIT_SUCCESS)` on FreeRTOS only.
- `paraos::jthread` does not expose a name getter, so debug output that uses `thread_.GiveName()` must capture the name separately or simplify the output.
- `paraos::Thread::DelayMs()` is not part of the std-like API. A cross-platform delay helper is needed for the tests; otherwise every test must `#ifdef` between `std::this_thread::sleep_for` (PC) and `vTaskDelay` (FreeRTOS).
- FreeRTOS `paraos::jthread` already signals a binary semaphore before `vTaskDelete(nullptr)`, which supports `join()`. The main risk is the interaction between `request_stop()`, blocking queue operations with timeouts, and destructor-issued `join()`.

---

## Audited Test Files

| # | File | CTest target | CTest label | Legacy thread primitive |
|---|------|--------------|-------------|-------------------------|
| 1 | `containers/tests/test_queue_blocking_spmc.cpp` | `test_queue_blocking_spmc` | `stress` | `paraos::Thread` |
| 2 | `containers/tests/test_queue_blocking_mpsc.cpp` | `test_queue_blocking_mpsc` | `stress` | `paraos::Thread` |
| 3 | `containers/tests/test_queue_blocking_mpmc.cpp` | `test_queue_blocking_mpmc` | `stress` | `paraos::Thread` |
| 4 | `containers/tests/test_multi_ringbuff_mpmc.cpp` | `test_multi_ringbuff_mpmc` | `stress` | `paraos::Thread` |
| 5 | `containers/tests/test_message_multithread_many_producer_many_consumers.cpp` | `test_message_multithread_many_producer_many_consumers` | `stress` | `paraos::Thread` |

All five targets are registered in `containers/tests/CMakeLists.txt` under `if(THREAD_ENABLE)` and carry the `stress` label. None of them links against `GTest::gtest_main`; they are standalone executables that return `0` (PC) or call `std::_Exit(EXIT_SUCCESS)` (FreeRTOS).

---

## Legacy Primitive Usage Per Test

### `test_queue_blocking_spmc.cpp`

- One global watcher thread: `paraos::Thread check_test_complete_and_exit` (lines 48–51).
- One `Producer` class that owns `paraos::Thread thread_` and registers `Producer::Run` via `RegisterDelegate()` in its constructor.
- Five `Consumer` classes, each owning `paraos::Thread thread_` and registered the same way.
- Worker bodies call `thread_.GiveName()`, `thread_.Finished()`, and `paraos::Thread::DelayMs()`.
- `ExitFromTest()` polls counters, calls `CheckIfTestSuccessfullyComplete()`, then `paraos::Thread::Exit()` (PC) or `std::_Exit(EXIT_SUCCESS)` (FreeRTOS).
- `main()` registers the watcher delegate, constructs static `Producer`/`Consumer` objects, then calls `StartScheduler()` and `DeleteAll()`.

### `test_queue_blocking_mpsc.cpp`

- Same watcher pattern as `spmc`.
- Five `Producer` classes, one `Consumer` class.
- Counters `total_producer_threads_numb` and `total_items_to_be_pushed` are computed after all producers are constructed.
- `ExitFromTest()` compares against `total_producer_threads_numb` and `consumer_thread_numb`.
- Otherwise identical lifecycle to `spmc`.

### `test_queue_blocking_mpmc.cpp`

- Same watcher pattern.
- Three `Consumer` classes and three `Producer` classes.
- `expected_total_items_in_queue = producers_total_numb * one_producer_expected_push_items_numb`.
- Worker bodies are shorter (single pop/push attempt with timeout) but use the same `Finished()`/`DelayMs()` pattern.

### `test_multi_ringbuff_mpmc.cpp`

- Same watcher pattern plus an additional `AssertsForTestComplete()` call **after** `StartScheduler()`/`DeleteAll()` in `main()`.
- Eight producers, each with a distinct `str_idx`.
- Eight consumers.
- `Producer::Run()` loops over `str_array`, writes a string, calls `ThreadExit()` (which increments `producer_thread_exit_cnt` and calls `thread_.Finished()`).
- `Consumer::Run()` reads once with a timeout; if it has already read the full byte count, it exits via `ThreadExit()`.
- Because `AssertsForTestComplete()` is called both in `ExitFromTest()` and after the scheduler returns, the migrated test should move the final assertion to after the inner `jthread` scope in `main()`.

### `test_message_multithread_many_producer_many_consumers.cpp`

- Same watcher pattern.
- Six consumers and six producers, many with explicit `attr.priority` values (`kLowest`, `kBelowNormal`, `kNormal`, `kAboveNormal`, `kHighest`).
- Producers write into a `MessageBuffer`, then push the string into `producers_str_container` under `paraos::CriticalSection`.
- Consumers pop from the buffer and push into `consumers_str_container` under `paraos::CriticalSection`.
- `CheckIfTestSuccessfullyComplete()` uses `assert()` to verify sizes and membership.
- This is the only audited test that sets per-thread priorities other than the default. `paraos::jthread` accepts `ThreadAttr` with `priority`, so the priority values are preserved.

---

## API Differences: `paraos::Thread` vs. `paraos::jthread`

| Topic | Legacy `paraos::Thread` | std-like `paraos::jthread` | Impact on tests |
|-------|------------------------|----------------------------|-----------------|
| **Construction** | `explicit Thread(const ThreadAttr& attr, bool thread_start_flag = true)` | `template <typename Function, typename... Args> explicit jthread(const ThreadAttr& attr, Function&& func, Args&&... args)` | The callable and its arguments are passed directly to the constructor; no separate `RegisterDelegate()` step. |
| **Delegate registration** | `void RegisterDelegate(thread_delegate_type run)` | Not needed | Remove `RegisterDelegate()` and `thread_delegate_type` usage. |
| **Thread start** | Threads are created but block until `StartScheduler()` is called | Threads start immediately | Timing/order may change slightly; tests must tolerate immediate execution. |
| **Run method** | Member `void Run()` bound by delegate | Any callable/functor/lambda | `Producer`/`Consumer` no longer need to own a `Thread`; they can be stateful functors or lambdas. |
| **Completion signal** | `void Finished(Base* deferred = nullptr)` | Return from the callable | Replace `thread_.Finished(); break;` with a plain `return;`. |
| **Scheduler lifecycle** | `static void StartScheduler()` / `static void DeleteAll()` | None | Remove both calls. |
| **Name getter** | `std::string_view GiveName() const` | No public getter; name stored in private `ThreadAttr attr_` | Capture the name locally if debug output needs it, or simplify output. |
| **Delay/yield** | `static void DelayMs(delay_type sleep_ms)` | No equivalent method | Need a cross-platform delay helper or `#ifdef` per platform. |
| **Program exit** | `static void Exit()`; on FreeRTOS tests call `std::_Exit(EXIT_SUCCESS)` | Destructor joins if joinable; no static exit helper | On FreeRTOS, call `std::_Exit(EXIT_SUCCESS)` after the inner scope; on PC, `return 0`. |
| **Ownership** | Static/global `Thread` objects | Local `std::vector<paraos::jthread>` | RAII join on scope exit replaces manual `DeleteAll()`. |
| **Stop token** | Not available | `paraos::stop_token` passed as last argument to callable | Not required by current tests, but available if future tests need cooperative cancellation. |
| **Join** | No explicit join API | `void join()` / destructor joins | Final assertions run after the inner scope destroys the vector. |

### Exact signatures from headers

From `port_pc/paraos_jthread.hpp`:

```cpp
// Default attribute constructor
template <typename Function, typename... Args>
  requires(!std::is_same_v<std::decay_t<Function>, ThreadAttr>)
explicit jthread(Function&& func, Args&&... args);

// With explicit ThreadAttr
template <typename Function, typename... Args>
explicit jthread(const ThreadAttr& attr, Function&& func, Args&&... args);

// Destructor
~jthread() {
  if (thread_.joinable()) {
    thread_.request_stop();
    thread_.join();
  }
}

void join();
[[nodiscard]] auto joinable() const noexcept -> bool;
[[nodiscard]] auto request_stop() noexcept -> bool;
```

From `port_freertos/paraos_jthread.hpp`:

```cpp
// Same two constructor templates as PC

// Destructor
~jthread() {
  if (joinable()) {
    request_stop();
    join();
  }
  delete context_;
}

void join() {
  if (context_ != nullptr) {
    context_->join_sem.Take();
  }
}

[[nodiscard]] auto joinable() const noexcept -> bool {
  return context_ != nullptr && context_->handle != nullptr;
}

[[nodiscard]] auto request_stop() noexcept -> bool {
  if (context_ != nullptr) {
    context_->stop_flag.store(true);
    return true;
  }
  return false;
}
```

The FreeRTOS task entry point is:

```cpp
static void RunTask(void* param) {
  auto* ctx = static_cast<Context*>(param);
  if (ctx != nullptr && ctx->invoker != nullptr) {
    ctx->invoker->Invoke(stop_token{&ctx->stop_flag});
  }
  if (ctx != nullptr) {
    ctx->join_sem.Give();
  }
  vTaskDelete(nullptr);
}
```

This ordering (invoke → give semaphore → delete) is the critical correctness property for `join()` on FreeRTOS.

---

## Synchronization Primitive Inventory

A grep across `containers/tests/*.cpp` for `paraos::Mutex`, `MutexGuard`, `paraos::SemaphoreBinary`, `paraos::SemaphoreCounting`, `paraos::mutex`, `paraos::binary_semaphore`, and `paraos::counting_semaphore` returned **no matches**.

Therefore:

- Phase 21 (synchronization migration) is expected to require **no test changes** in `containers/tests/` unless hidden dependencies surface during implementation.
- The existing `paraos::CriticalSection` usage for `std::cout` and shared `std::vector` updates should remain, per user decision D-09/D-10.
- A compile-time smoke test confirming `std::lock_guard<paraos::mutex>` and `std::unique_lock<paraos::mutex>` on all ports is still valuable, but it can be a small standalone verification rather than a test rewrite.

### `paraos::mutex` compatibility check

Both implementations satisfy the `BasicLockable` requirements:

- PC: `void lock(); [[nodiscard]] auto try_lock() -> bool; void unlock();`
- FreeRTOS: same signatures over `xSemaphoreCreateMutex()` / `xSemaphoreTake` / `xSemaphoreGive`.

This makes `std::lock_guard<paraos::mutex>` and `std::unique_lock<paraos::mutex>` valid on both ports.

---

## FreeRTOS Gaps / Hardening Candidates

| # | Area | Current implementation | Concern for migrated tests | Suggested verification / hardening |
|---|------|------------------------|----------------------------|------------------------------------|
| 1 | `~jthread()` / `join()` | `RunTask` signals `join_sem` then `vTaskDelete(nullptr)` | Correctness of join after functor returns; multiple joiners not supported; `joinable()` does not clear `handle` after task deletion | Add a test or inspect that destructor-issued `join()` returns after a short-lived task. Consider resetting `context_->handle = nullptr` after join to make `joinable()` safe, but verify no use-after-free. |
| 2 | `request_stop()` / `stop_token` | `stop_flag` is `etl::atomic_bool*` inside `Context` | Setting the flag does not wake a task blocked on a queue/semaphore. In migrated tests, workers block in `queue.Pop(timeout)` or `multi_ring_buff.Read(..., delay_ms)` with finite timeouts, so destructor `join()` will wait at most until the timeout. | Verify that CTest `TIMEOUT 10` is sufficient and that finite timeouts prevent deadlock. If cooperative stop is required, the queue/buffer APIs would need interruptible waits. |
| 3 | `counting_semaphore::release(N)` | `vTaskSuspendAll()` + loop of `xSemaphoreGive()` | Assumes `GetCurrentCount() + update <= max()`. Tests do not currently use semaphores, but Phase 22 may verify behavior. | Keep existing implementation; add a compile/runtime test only if Phase 21 discovers semaphore usage. |
| 4 | `try_acquire_for` | Converts duration to `ms`, then `PARAOS_ConvertMsToTicks` | Tick conversion rounding and overflow for very small durations. | Verify with typical test timeouts (1 ms, 5 ms, 2000 ms) that the resulting tick count is non-zero and bounded. |
| 5 | Thread name lifetime | `xTaskCreate(..., attr.thread_name.data(), ...)` | `std::string_view` data must outlive `xTaskCreate`. Current tests use string literals, which is safe. | Document the contract; if dynamic names are ever used, ensure the backing string outlives construction. |
| 6 | Cross-platform delay helper | `paraos::Thread::DelayMs()` exists only in legacy API | Migrated tests need a uniform way to yield/sleep. | Either introduce a small `paraos::sleep_for(std::chrono::milliseconds)` wrapper, or use `#ifdef PARAOS_LIKE_FREERTOS` with `vTaskDelay(PARAOS_ConvertMsToTicks(ms))` and `std::this_thread::sleep_for` otherwise. |
| 7 | `std::chrono` usage | `try_acquire_for` uses `std::chrono::duration`; `jthread` constructors do not use chrono | FreeRTOS toolchain must support `<chrono>` duration types. | Confirm that the FreeRTOS presets compile `paraos::counting_semaphore` and any new delay helper. |

The highest-priority items for Phase 22 are **#1** (`join()` correctness) and **#6** (delay helper), because they directly affect whether the migrated multithreaded tests can run on FreeRTOS without deadlocks or portability `#ifdef` blocks in every test.

---

## Migration Sketches

### Common `main()` skeleton after migration

```cpp
auto main() -> int {
  {
    std::vector<paraos::jthread> threads;

    // Push producers and consumers.
    threads.emplace_back(paraos::ThreadAttr{"Prod 0", ...}, Producer{0});
    threads.emplace_back(paraos::ThreadAttr{"--Cons 0", ...}, Consumer{});
    // ...

    // Destructor of 'threads' joins all workers here.
  }

  CheckIfTestSuccessfullyComplete();

#ifdef PARAOS_LIKE_FREERTOS
  std::_Exit(EXIT_SUCCESS);
#else
  return 0;
#endif
}
```

Key points:

- The `std::vector<paraos::jthread>` is inside an explicit inner scope so that destructors run before final assertions and `_Exit()`.
- The watcher thread and `ExitFromTest()` delegate are removed entirely.
- `StartScheduler()` and `DeleteAll()` are removed.

### `Producer`/`Consumer` after migration

Using lambdas:

```cpp
struct Producer {
  explicit Producer(std::size_t str_idx) : str_idx_{str_idx} {}

  void operator()(const paraos::stop_token& /*token*/) {
    // ... same logic as Run() but return instead of Finished()
  }

 private:
  std::size_t str_idx_{};
};
```

Or, more idiomatically for simple tests, replace the struct with a capturing lambda created directly in `main()`:

```cpp
threads.emplace_back(paraos::ThreadAttr{"Prod 0", ...},
                     [&queue, &push_item_cnt](const paraos::stop_token&) {
                       // producer logic
                     });
```

The choice between struct functors and lambdas is left to Phase 20; the research only documents that both are possible.

### Per-test gotchas

| Test | Gotcha |
|------|--------|
| `test_queue_blocking_spmc.cpp` | Remove watcher; move final assertion after inner scope. |
| `test_queue_blocking_mpsc.cpp` | Compute `total_items_to_be_pushed` before constructing threads, or capture the precomputed value. |
| `test_queue_blocking_mpmc.cpp` | Compute `expected_total_items_in_queue` before constructing threads. |
| `test_multi_ringbuff_mpmc.cpp` | Remove the duplicate `AssertsForTestComplete()` after `StartScheduler()`; keep only the one after the inner scope. |
| `test_message_multithread_many_producer_many_consumers.cpp` | Preserve `attr.priority` values in `ThreadAttr`. Keep `paraos::CriticalSection` around shared `std::vector` updates. |

---

## Validation Architecture

Because Phase 19 does not modify source code, the validation strategy is review-centric rather than test-centric.

### Static / review verification

- Every audited test file is listed in the inventory table.
- Every `paraos::Thread` usage site is mapped to a std-like replacement or marked "remove".
- API-differences table contains exact constructor, join, destructor, and `request_stop()` signatures from both PC and FreeRTOS headers.
- FreeRTOS gap list is ranked and actionable.
- Migration sketches compile syntactically and follow the agreed pattern (inner scope, `std::vector<paraos::jthread>`, `_Exit()` only on FreeRTOS).

### Manual verification

- Peer review of the inventory to catch missed `paraos::Thread` patterns.
- Confirm with the user that Phase 21 synchronization work can be minimal.
- Confirm that the proposed delay-helper approach (wrapper vs. per-test `#ifdef`) is acceptable.

### Nyquist sampling

- Sampling rate: review one test migration sketch per task commit.
- Max feedback latency: one review cycle per test.
- No watch-mode flags; all verification is source-review or command-based.

---

## Open Questions to Resolve During Execution

1. Should Phase 20 use capturing lambdas for `Producer`/`Consumer`, or keep small struct functors?
2. Should Phase 22 introduce a cross-platform `paraos::sleep_for` helper, or should each test use platform `#ifdef` for delays?
3. Does the FreeRTOS `jthread` destructor behave correctly when the task has already self-deleted before `~jthread()` runs?
4. Are there any hidden uses of `paraos::Mutex` / semaphore in macros or included headers inside the five tests?

---

## References

- `.planning/REQUIREMENTS.md` — ANL-01, ANL-02, ANL-03
- `.planning/ROADMAP.md` — Phase 19 goal and success criteria
- `.planning/phases/19-inventory-gap-analysis/19-CONTEXT.md` — user decisions from discuss-phase
- `containers/tests/CMakeLists.txt`
- `containers/tests/test_queue_blocking_spmc.cpp`
- `containers/tests/test_queue_blocking_mpsc.cpp`
- `containers/tests/test_queue_blocking_mpmc.cpp`
- `containers/tests/test_multi_ringbuff_mpmc.cpp`
- `containers/tests/test_message_multithread_many_producer_many_consumers.cpp`
- `port_pc/paraos_jthread.hpp`
- `port_freertos/paraos_jthread.hpp`
- `port_pc/paraos_mutex_std.hpp`
- `port_freertos/paraos_mutex_std.hpp`
- `port_pc/paraos_semaphore_std.hpp`
- `port_freertos/paraos_semaphore_std.hpp`
- `port_unix/paraos_thread.hpp` — legacy API reference
- `paraos_thread_common.hpp`
