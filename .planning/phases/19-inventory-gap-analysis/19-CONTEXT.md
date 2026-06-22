# Phase 19: Inventory & gap analysis - Context

**Gathered:** 2026-06-22
**Status:** Ready for planning

<domain>
## Phase Boundary

This phase delivers an audit of multithreaded container tests in `containers/tests/`, a documented mapping of legacy primitives (`paraos::Thread`) to std-like replacements (`paraos::jthread`, `paraos::mutex`, `paraos::*_semaphore`), and a concrete list of `port_freertos` gaps needed for uniform behavior across PC and FreeRTOS.

The phase does **not** implement the migration itself — that is covered by Phases 20–23.

</domain>

<decisions>
## Implementation Decisions

### Test termination pattern
- **D-01 [informational]:** The watcher thread (`check_test_complete_and_exit`) is removed from all multithreaded container tests.
- **D-02 [informational]:** `main()` is responsible for waiting on all worker threads and running the final assertions.
- **D-03 [informational]:** The global test timeout (10 seconds) is enforced through CTest via `TIMEOUT 10` in `containers/tests/CMakeLists.txt`, not inside `join()` or test logic.

### jthread lifetime management
- **D-04 [informational]:** Worker threads are stored in a local `std::vector<paraos::jthread>` inside `main()`.
- **D-05 [informational]:** `Producer`/`Consumer` classes become ordinary functors or lambdas that capture their index/name/state; they no longer own a `paraos::Thread` object.

### FreeRTOS test finalization
- **D-06 [informational]:** The `jthread` vector is placed in an explicit inner scope in `main()` so that destructors (and therefore `join()`) run before the final `_Exit()`.
- **D-07 [informational]:** On FreeRTOS, after the inner scope exits, `main()` calls `std::_Exit(EXIT_SUCCESS)` to terminate the test.
- **D-08 [informational]:** On PC, `main()` returns `0` normally after the inner scope exits.

### Synchronization primitives in container tests
- **D-09 [informational]:** `paraos::CriticalSection` remains in place everywhere it currently exists in the multithreaded container tests.
- **D-10 [informational]:** Replacing `CriticalSection` with `paraos::mutex` / `std::lock_guard` is out of scope for this milestone unless implementation reveals a hard requirement.

### Output artifacts
- **D-11 [informational]:** The inventory is captured as a structured table inside this CONTEXT.md and will be reproduced in the phase plan; no separate standalone migration document is required unless the planner decides it aids review.

### Claude's Discretion
- None — all key choices were made by the user.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Requirements and roadmap
- `.planning/REQUIREMENTS.md` — v1.5 requirements ANL-01..ANL-03 and downstream THR-*, SYNC-*, FR-*, BLD-* requirements.
- `.planning/ROADMAP.md` — Phase 19 goal, success criteria, and dependency chain (Phases 20–23).

### Existing std-like primitives
- `port_pc/paraos_jthread.hpp` — PC/Unix/Windows `paraos::jthread` implementation.
- `port_freertos/paraos_jthread.hpp` — FreeRTOS `paraos::jthread` implementation.
- `port_pc/paraos_mutex_std.hpp` — PC/Unix/Windows `paraos::mutex` implementation.
- `port_freertos/paraos_mutex_std.hpp` — FreeRTOS `paraos::mutex` implementation.
- `port_pc/paraos_semaphore_std.hpp` — PC/Unix/Windows `paraos::counting_semaphore` implementation.
- `port_freertos/paraos_semaphore_std.hpp` — FreeRTOS `paraos::counting_semaphore` implementation.
- `paraos_thread_common.hpp` — `ThreadAttr`, `ThreadPriority`, and shared definitions.

### Container tests to audit
- `containers/tests/CMakeLists.txt` — how multithreaded tests are built, registered, and labeled (`stress`).
- `containers/tests/test_queue_blocking_spmc.cpp` — legacy `paraos::Thread` + `StartScheduler()` usage.
- `containers/tests/test_queue_blocking_mpsc.cpp` — legacy `paraos::Thread` + `StartScheduler()` usage.
- `containers/tests/test_queue_blocking_mpmc.cpp` — legacy `paraos::Thread` + `StartScheduler()` usage.
- `containers/tests/test_multi_ringbuff_mpmc.cpp` — legacy `paraos::Thread` + `StartScheduler()` usage.
- `containers/tests/test_message_multithread_many_producer_many_consumers.cpp` — legacy `paraos::Thread` + `StartScheduler()` usage.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `paraos::jthread` with `ThreadAttr` constructor — already supports name, stack, and priority, which matches every `paraos::ThreadAttr` usage in the container tests.
- `paraos::CriticalSection` — continues to be usable for protecting `std::cout` and shared `std::vector` in tests.
- `std::vector<paraos::jthread>` — idiomatic RAII container for owning and joining worker threads.

### Established Patterns
- Legacy tests use `RegisterDelegate()` + member `Run()` inside `Producer`/`Consumer` classes; the new pattern will use capturing lambdas or small functors passed directly to `paraos::jthread`.
- FreeRTOS tests currently rely on `std::_Exit(EXIT_SUCCESS)` to terminate; this is preserved but moved to the end of `main()` after the explicit `jthread` scope.
- All multithreaded container tests are registered with CTest label `stress`.

### Integration Points
- `containers/tests/CMakeLists.txt` must add `TIMEOUT 10` to each multithreaded test target.
- `paraos::jthread` on FreeRTOS uses a `SemaphoreBinary join_sem` for `join()`; the planner should verify that `RunTask` always signals it before `vTaskDelete(nullptr)`.

### Inventory of legacy primitives per test

| Test | Legacy primitive(s) | Observed API calls | std-like replacement | Action |
|------|---------------------|--------------------|----------------------|--------|
| `test_queue_blocking_spmc.cpp` | `paraos::Thread` | constructor, `RegisterDelegate`, `GiveName`, `Finished`, `StartScheduler`, `DeleteAll`, `Exit`, `DelayMs` | `paraos::jthread` | Remove watcher thread; move final assertion to `main()` after inner scope; remove `StartScheduler`/`DeleteAll`/`Exit`. |
| `test_queue_blocking_mpsc.cpp` | `paraos::Thread` | same set as spmc | `paraos::jthread` | Compute total items before creating threads; otherwise same as spmc. |
| `test_queue_blocking_mpmc.cpp` | `paraos::Thread` | same set as spmc/mpsc | `paraos::jthread` | Compute expected items before creating threads; otherwise same pattern. |
| `test_multi_ringbuff_mpmc.cpp` | `paraos::Thread` | same set plus `AssertsForTestComplete` after scheduler | `paraos::jthread` | Remove duplicate post-scheduler assertion; keep only the one after the inner scope. |
| `test_message_multithread_many_producer_many_consumers.cpp` | `paraos::Thread` | same set plus per-thread `priority` values | `paraos::jthread` | Preserve `ThreadAttr.priority`; keep `CriticalSection` around shared containers. |

**Observation:** A grep across `containers/tests/*.cpp` for `paraos::Mutex`, `MutexGuard`, `paraos::SemaphoreBinary`, `paraos::SemaphoreCounting`, `paraos::mutex`, `paraos::binary_semaphore`, and `paraos::counting_semaphore` returned **no matches**. Phase 21 (SYNC-01..SYNC-03) is therefore expected to require no changes in `containers/tests/` unless hidden dependencies surface during implementation.

### API gaps between `paraos::Thread` and `paraos::jthread` relevant to container tests

| Legacy API / behavior | Exact legacy signature (port_unix) | std-like equivalent | Exact std-like signature | Status / Gap |
|-----------------------|------------------------------------|---------------------|--------------------------|--------------|
| Construction | `explicit Thread(const ThreadAttr& attr, bool thread_start_flag = true)` | `jthread(attr, func, args...)` | `template <typename Function, typename... Args> explicit jthread(const ThreadAttr& attr, Function&& func, Args&&... args)` | Available; `ThreadAttr` is forwarded unchanged. |
| Default construction | same | `jthread(func, args...)` | `template <typename Function, typename... Args> requires(!std::is_same_v<std::decay_t<Function>, ThreadAttr>) explicit jthread(Function&& func, Args&&... args)` | Available. |
| Delegate registration | `void RegisterDelegate(thread_delegate_type run)` | None | Callable passed directly to constructor | Remove `RegisterDelegate` and `thread_delegate_type`. |
| Thread start | `static void StartScheduler()` | Immediate start on construction | — | Remove call; timing semantics change slightly. |
| Completion signal | `void Finished(Base* deferred = nullptr)` | Return from callable | — | Replace `thread_.Finished(); break;` with `return;`. |
| Cleanup | `static void DeleteAll()` / `static void Exit()` | Destructor joins | `~jthread()` calls `request_stop()` then `join()` if joinable | Remove `DeleteAll()`/`Exit()`; use inner scope and RAII join. |
| Name getter | `std::string_view GiveName() const` | No public getter | Name stored in private `ThreadAttr attr_` | Capture name locally if needed for debug output. |
| Delay/yield | `static void DelayMs(delay_type sleep_ms)` | No equivalent | — | **Gap:** need cross-platform delay helper or per-test `#ifdef` between `std::this_thread::sleep_for` and `vTaskDelay`. |
| Join | Not provided | `void join()` | `void join()` | New API; destructor uses it automatically. |
| Stop token | Not available | Passed as last argument | `void operator()(const paraos::stop_token&)` | Not used by current tests. |

### FreeRTOS gaps / hardening candidates (for Phase 22)

| # | Area | Concern | Suggested verification / hardening |
|---|------|---------|------------------------------------|
| 1 | `~jthread()` / `join()` | `RunTask` signals `join_sem` then calls `vTaskDelete(nullptr)`. Correctness depends on semaphore signal happening before task deletion; multiple joiners are not supported. | Add a focused runtime/inspection check that destructor-issued `join()` returns after a short-lived task. Consider whether `context_->handle` should be nulled after join. |
| 2 | `request_stop()` / `stop_token` | `stop_flag` is visible, but setting it does not wake a blocked task. Migrated workers block in queue/buffer operations with finite timeouts, so destructor `join()` will wait at most until the timeout. | Verify that CTest `TIMEOUT 10` prevents hangs; if cooperative cancellation is needed, queue/buffer waits must become interruptible. |
| 3 | `counting_semaphore::release(N)` | Implemented with `vTaskSuspendAll()` + loop of `xSemaphoreGive()`. | No change required unless Phase 21 discovers semaphore usage. |
| 4 | `try_acquire_for` | Converts `std::chrono::duration` to ms then `PARAOS_ConvertMsToTicks`. | Verify tick conversion for typical test timeouts (1 ms, 5 ms, 2000 ms). |
| 5 | Thread name lifetime | `xTaskCreate(..., attr.thread_name.data(), ...)` requires the backing string to outlive task creation. | Document contract; current tests use string literals, which is safe. |
| 6 | Cross-platform delay | `paraos::Thread::DelayMs()` is not part of the std-like API. | **Top candidate:** introduce `paraos::sleep_for(std::chrono::milliseconds)` wrapper, or accept per-test `#ifdef` in Phase 20. |
| 7 | `std::chrono` support | FreeRTOS `try_acquire_for` already uses `<chrono>`. | Confirm FreeRTOS presets compile the semaphore header and any new delay helper. |

**Priority:** #1 (`join()` correctness) and #6 (delay helper) are the highest-impact items for the migrated tests.

</code_context>

<specifics>
## Specific Ideas

- Target test structure after migration:
  ```cpp
  auto main() -> int {
    {
      std::vector<paraos::jthread> threads;
      // push producers and consumers
      // destructor joins all threads here
    }
#ifdef PARAOS_LIKE_FREERTOS
    std::_Exit(EXIT_SUCCESS);
#else
    return 0;
#endif
  }
  ```
- CTest timeout per test: add `TIMEOUT 10` to each of the five multithreaded tests in `containers/tests/CMakeLists.txt`.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 19-Inventory & gap analysis*
*Context gathered: 2026-06-22*
