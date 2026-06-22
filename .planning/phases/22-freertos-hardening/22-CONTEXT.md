# Phase 22: FreeRTOS std-like primitives hardening - Context

**Gathered:** 2026-06-22
**Status:** Ready for planning
**Mode:** Auto-generated (decisions inherited from Phase 19/20/21)

<domain>
## Phase Boundary

Phase 22 adds the minimum FreeRTOS-specific hardening needed for the migrated container tests to behave uniformly across PC and FreeRTOS. The highest-priority items identified in Phase 19 are:

1. `paraos::jthread` destructor / `join()` correctness on FreeRTOS.
2. A cross-platform delay helper to replace the local `#ifdef` blocks in the migrated tests.

Scope is limited to `port_freertos/` and optionally a new shared header; test files are not rewritten except to adopt the new helper.

</domain>

<decisions>
## Implementation Decisions

### Cross-platform delay helper
- **D-01:** Add a public `paraos::sleep_for(std::chrono::milliseconds)` helper in a new header `paraos_sleep.hpp`, implemented in `port_pc/`, `port_unix/`, `port_win/`, and `port_freertos/` (or via `#ifdef` inside a single header).
- **D-02:** The helper must compile on C++17 (the project minimum) and therefore accept `std::chrono::milliseconds` rather than a generic `std::chrono::duration` template, to avoid C++20 constraints.
- **D-03:** On PC/Unix/Windows the helper uses `std::this_thread::sleep_for`. On FreeRTOS it uses `vTaskDelay(PARAOS_ConvertMsToTicks(ms))`.

### FreeRTOS jthread join safety
- **D-04 [informational]:** `port_freertos/paraos_jthread.hpp` already signals `join_sem` in `RunTask` before `vTaskDelete(nullptr)`. This satisfies the basic join contract for the migrated tests.
- **D-05 [informational]:** Multiple concurrent joiners are not required by the tests; destructor-issued join is the only scenario.
- **D-06 [informational]:** No changes to `join()` signaling order are required unless Phase 22 testing reveals a defect.

### Thread name / stop_token / chrono
- **D-07 [informational]:** Thread names are passed as string literals in tests, so name lifetime is safe.
- **D-08 [informational]:** `stop_token` is not used cooperatively in the migrated tests; destructor-issued `request_stop()` is sufficient.
- **D-09 [informational]:** `std::chrono` compatibility is already demonstrated by the semaphore smoke tests compiling on FreeRTOS.

### Claude's Discretion
- The exact file placement of `paraos_sleep.hpp` and whether it is a single header with `#ifdef` or a port-specific file is left to implementation convenience, provided all ports compile.

</decisions>

<canonical_refs>
## Canonical References

- `.planning/phases/19-inventory-gap-analysis/19-RESEARCH.md` — FreeRTOS gap analysis.
- `.planning/phases/20-migrate-thread-primitives/20-SUMMARY.md` — migrated tests using local `SleepMs()` helpers.
- `port_freertos/paraos_jthread.hpp` — FreeRTOS jthread implementation.
- `port_freertos/paraos_utils.hpp` — `PARAOS_ConvertMsToTicks`.
- `port_pc/paraos_jthread.hpp` — PC jthread implementation.

</canonical_refs>

<code_context>
## Existing Code Insights

### Local SleepMs pattern in migrated tests
Each migrated test currently duplicates:
```cpp
inline void SleepMs(paraos::delay_type sleep_ms) {
#ifdef PARAOS_LIKE_FREERTOS
  vTaskDelay(paraos::PARAOS_ConvertMsToTicks(sleep_ms));
#else
  std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
#endif
}
```

### FreeRTOS jthread join signaling
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

</code_context>

<specifics>
## Specific Ideas

- Create `paraos_sleep.hpp` in the project root with:
```cpp
#ifndef PARAOS_SLEEP_HPP
#define PARAOS_SLEEP_HPP

#include <chrono>

#include "paraos_utils.hpp"

#ifdef PARAOS_LIKE_FREERTOS
#include "task.h"
#endif

namespace paraos {

inline void sleep_for(std::chrono::milliseconds duration) {
#ifdef PARAOS_LIKE_FREERTOS
  vTaskDelay(PARAOS_ConvertMsToTicks(
      static_cast<delay_type>(duration.count())));
#else
  std::this_thread::sleep_for(duration);
#endif
}

}  // namespace paraos

#endif  // PARAOS_SLEEP_HPP
```
- Replace the local `SleepMs()` helpers in the five migrated tests with `paraos::sleep_for(std::chrono::milliseconds(ms))`.
- Include `paraos_sleep.hpp` where needed.
- Keep `task.h` include guarded by `PARAOS_LIKE_FREERTOS`.

</specifics>

<deferred>
## Deferred Ideas

- Generic `std::chrono::duration` overloads requiring C++20 — deferred until the project adopts C++20 as the minimum.
- Changes to `joinable()` semantics or multiple-joiner support — not required by current tests.

</deferred>

---

*Phase: 22-freertos-hardening*
*Context gathered: 2026-06-22 via autonomous smart-discuss (decisions inherited from Phase 19/20/21)*
