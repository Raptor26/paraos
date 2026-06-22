# Phase 20: Migrate thread primitives in container tests - Context

**Gathered:** 2026-06-22
**Status:** Ready for planning
**Mode:** Auto-generated (decisions inherited from Phase 19)

<domain>
## Phase Boundary

Phase 20 rewrites the five multithreaded container tests in `containers/tests/` so they use `paraos::jthread` instead of the legacy `paraos::Thread` API. This phase covers only the thread primitive migration; synchronization primitive migration (if any) is Phase 21, and FreeRTOS hardening is Phase 22.

Tests to migrate:
- `containers/tests/test_queue_blocking_spmc.cpp`
- `containers/tests/test_queue_blocking_mpsc.cpp`
- `containers/tests/test_queue_blocking_mpmc.cpp`
- `containers/tests/test_multi_ringbuff_mpmc.cpp`
- `containers/tests/test_message_multithread_many_producer_many_consumers.cpp`

</domain>

<decisions>
## Implementation Decisions

### Worker structure
- **D-01 [informational]:** Keep existing `Producer` and `Consumer` structs, but remove the `paraos::Thread` member and `RegisterDelegate()` call. Convert `Run()` to `operator()(const paraos::stop_token&)`. This minimizes the diff and preserves the existing test logic.

### Thread ownership
- **D-02 [informational]:** Store all worker threads in a single local `std::vector<paraos::jthread>` inside an explicit inner scope in `main()`.
- **D-03 [informational]:** Use `threads.emplace_back(paraos::ThreadAttr{...}, Worker{...});` to construct each worker and its thread atomically.

### Test finalization
- **D-04 [informational]:** The inner scope in `main()` ends, destroying the vector and joining all threads before any final assertions run.
- **D-05 [informational]:** Final assertions (`CheckIfTestSuccessfullyComplete`, `AssertsForTestComplete`) run after the inner scope.
- **D-06 [informational]:** On FreeRTOS, call `std::_Exit(EXIT_SUCCESS)` after the final assertions. On PC, return `0`.

### Watcher thread removal
- **D-07 [informational]:** Remove the `check_test_complete_and_exit` watcher thread and the `ExitFromTest()` delegate entirely in all five tests.

### Scheduler lifecycle removal
- **D-08 [informational]:** Remove all calls to `paraos::Thread::StartScheduler()` and `paraos::Thread::DeleteAll()`.

### Completion signal replacement
- **D-09 [informational]:** Replace `thread_.Finished(); break;` with plain `return;` from the worker functor.

### Debug output
- **D-10 [informational]:** Because `paraos::jthread` has no `GiveName()` getter, capture the thread name locally in the worker via a `std::string_view` or `const char*` member, or simplify the debug output to use a locally known label. Prefer keeping the name for trace readability.

### Delay helper
- **D-11 [informational]:** For Phase 20, replace `paraos::Thread::DelayMs(ms)` with a local inline helper or a direct platform call:
  - On PC/Unix/Windows: `std::this_thread::sleep_for(std::chrono::milliseconds(ms));`
  - On FreeRTOS: `vTaskDelay(PARAOS_ConvertMsToTicks(ms));`
- **D-12 [informational]:** A cross-platform `paraos::sleep_for` wrapper is explicitly deferred to Phase 22; Phase 20 uses local `#ifdef` blocks to avoid blocking on Phase 22.

### Priorities
- **D-13 [informational]:** Preserve all explicit `ThreadAttr.priority` values from `test_message_multithread_many_producer_many_consumers.cpp`.

### CriticalSection
- **D-14 [informational]:** Keep all existing `paraos::CriticalSection` usage unchanged.

### CTest timeout
- **D-15 [informational]:** Add `TIMEOUT 10` to each of the five multithreaded test targets in `containers/tests/CMakeLists.txt`.

### Claude's Discretion
- None — all key choices are inherited from Phase 19 or specified above.

</decisions>

<canonical_refs>
## Canonical References

- `.planning/phases/19-inventory-gap-analysis/19-RESEARCH.md` — full audit and migration sketches.
- `.planning/phases/19-inventory-gap-analysis/19-CONTEXT.md` — Phase 19 decisions and API-difference tables.
- `port_pc/paraos_jthread.hpp` — PC `paraos::jthread` signatures.
- `port_freertos/paraos_jthread.hpp` — FreeRTOS `paraos::jthread` implementation.
- `paraos_thread_common.hpp` — `ThreadAttr`, `ThreadPriority`.
- `port_freertos/paraos_utils.hpp` — `PARAOS_ConvertMsToTicks`.
</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable patterns from Phase 19
- `paraos::jthread(attr, callable)` starts immediately and joins on destruction.
- Worker functors receive a `paraos::stop_token` as their last argument.
- `std::vector<paraos::jthread>` supports `emplace_back` with move-only `jthread`.

### Files to modify
- `containers/tests/test_queue_blocking_spmc.cpp`
- `containers/tests/test_queue_blocking_mpsc.cpp`
- `containers/tests/test_queue_blocking_mpmc.cpp`
- `containers/tests/test_multi_ringbuff_mpmc.cpp`
- `containers/tests/test_message_multithread_many_producer_many_consumers.cpp`
- `containers/tests/CMakeLists.txt` (add `TIMEOUT 10`)

</code_context>

<specifics>
## Specific Ideas

### Common worker skeleton
```cpp
struct Producer {
  explicit Producer(std::string_view name, std::size_t str_idx)
      : name_{name}, str_idx_{str_idx} {}

  void operator()(const paraos::stop_token& /*token*/) {
    // ... existing Run() logic, but use name_ instead of thread_.GiveName()
    // ... replace thread_.Finished(); break; with return;
  }

 private:
  std::string_view name_;
  std::size_t str_idx_;
};
```

### Common main() skeleton
```cpp
auto main() -> int {
  {
    std::vector<paraos::jthread> threads;
    threads.emplace_back(
        paraos::ThreadAttr{"Prod 0", paraos::GetStackMinimumSizeInBytes(),
                           paraos::ThreadPriority::kNormal, nullptr},
        Producer{"Prod 0", 0});
    // ... consumers ...
  }

  CheckIfTestSuccessfullyComplete();

#ifdef PARAOS_LIKE_FREERTOS
  std::_Exit(EXIT_SUCCESS);
#else
  return 0;
#endif
}
```

### Delay helper local macro (per-test, Phase 20 only)
```cpp
#ifdef PARAOS_LIKE_FREERTOS
#include "task.h"
inline void SleepMs(paraos::delay_type ms) {
  vTaskDelay(PARAOS_ConvertMsToTicks(ms));
}
#else
#include <thread>
#include <chrono>
inline void SleepMs(paraos::delay_type ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
#endif
```
</specifics>

<deferred>
## Deferred Ideas

- Cross-platform `paraos::sleep_for` wrapper — deferred to Phase 22.
- Replacing `paraos::CriticalSection` with `paraos::mutex` — deferred/out of scope per Phase 19 D-09/D-10.
</deferred>

---

*Phase: 20-migrate-thread-primitives*
*Context gathered: 2026-06-22 via autonomous smart-discuss (decisions inherited from Phase 19)*
