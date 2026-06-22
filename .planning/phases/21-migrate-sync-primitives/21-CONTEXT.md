# Phase 21: Migrate synchronization primitives in container tests - Context

**Gathered:** 2026-06-22
**Status:** Ready for planning
**Mode:** Auto-generated (decisions inherited from Phase 19/20)

<domain>
## Phase Boundary

Phase 21 replaces legacy synchronization primitives (`paraos::Mutex`, `MutexGuard`, `paraos::SemaphoreBinary`, `paraos::SemaphoreCounting`) with std-like equivalents (`paraos::mutex`, `paraos::binary_semaphore`, `paraos::counting_semaphore`) in the multithreaded container tests where semantically appropriate.

Scope is limited to `containers/tests/`.

</domain>

<decisions>
## Implementation Decisions

### Inventory result
- **D-01 [informational]:** The Phase 19 audit found no usage of `paraos::Mutex`, `MutexGuard`, `paraos::SemaphoreBinary`, or `paraos::SemaphoreCounting` in the five migrated multithreaded container tests.

### CriticalSection policy
- **D-02 [informational]:** All existing `paraos::CriticalSection` usage remains unchanged; replacing it with `paraos::mutex` is out of scope for this milestone.

### Verification approach
- **D-03 [informational]:** Because no legacy synchronization primitives remain in the target tests, Phase 21 focuses on a compile-time smoke test proving `std::lock_guard<paraos::mutex>` and `std::unique_lock<paraos::mutex>` work on all ports.
- **D-04 [informational]:** If hidden `paraos::Mutex` / semaphore usage is discovered during implementation, it will be replaced with the std-like equivalent and documented.

### Claude's Discretion
- None — all key choices are inherited from prior phases.

</decisions>

<canonical_refs>
## Canonical References

- `.planning/phases/19-inventory-gap-analysis/19-RESEARCH.md` — audit showing no legacy sync primitives in target tests.
- `.planning/phases/20-migrate-thread-primitives/20-SUMMARY.md` — migrated tests.
- `port_pc/paraos_mutex_std.hpp` — PC `paraos::mutex`.
- `port_freertos/paraos_mutex_std.hpp` — FreeRTOS `paraos::mutex`.
- `port_pc/paraos_semaphore_std.hpp` — PC `paraos::*_semaphore`.
- `port_freertos/paraos_semaphore_std.hpp` — FreeRTOS `paraos::*_semaphore`.
</canonical_refs>

<code_context>
## Existing Code Insights

### Target files
- `containers/tests/test_queue_blocking_spmc.cpp`
- `containers/tests/test_queue_blocking_mpsc.cpp`
- `containers/tests/test_queue_blocking_mpmc.cpp`
- `containers/tests/test_multi_ringbuff_mpmc.cpp`
- `containers/tests/test_message_multithread_many_producer_many_consumers.cpp`

### Observation
All five files use `paraos::CriticalSection` but not `paraos::Mutex`, `MutexGuard`, `paraos::SemaphoreBinary`, or `paraos::SemaphoreCounting`.

</code_context>

<specifics>
## Specific Ideas

- Add a small compile-time smoke test file `containers/tests/test_mutex_smoke.cpp` that:
  - Creates a `paraos::mutex`.
  - Locks it with `std::lock_guard<paraos::mutex>`.
  - Locks it with `std::unique_lock<paraos::mutex>` and calls `unlock()`.
  - Registers as a CTest test under the existing `test_paraos_containers` GTest executable or as a standalone executable.
- Alternatively, add a GTest test case to the existing `test_paraos_containers` executable verifying the same.
</specifics>

<deferred>
## Deferred Ideas

- Replacing `paraos::CriticalSection` with `paraos::mutex` — out of scope per user decision D-09/D-10.
</deferred>

---

*Phase: 21-migrate-sync-primitives*
*Context gathered: 2026-06-22 via autonomous smart-discuss (decisions inherited from Phase 19/20)*
