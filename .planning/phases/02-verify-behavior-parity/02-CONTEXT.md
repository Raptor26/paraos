# Phase 2: Verify Behavior Parity — Context

**Gathered:** 2026-06-20
**Status:** Ready for planning
**Source:** Synthesized from Phase 1 research, Phase 1 summary, and current `ctest` failures on macOS.

## Phase Boundary

This phase makes the public OSAL primitives exposed by `port_unix` behave identically on macOS and Linux. Phase 1 fixed compilation; Phase 2 fixes runtime behavior so the existing test suite produces the same results on both platforms.

## Implementation Decisions

### Locked decisions

- **Keep all public API signatures unchanged.** No changes to class names, method signatures, or `enum class ThreadPriority` values.
- **Keep platform isolation inside `port_unix/` using `#ifdef __linux__` / `#ifdef __APPLE__`.** The only non-port_unix edits allowed are the minimal `__APPLE__` additions already made in Phase 1 to `paraos_check.h` and `paraos_runtime_profiler.hpp`.
- **Map `ThreadPriority` enum values to the macOS `SCHED_RR` range inside `port_unix/paraos_thread.hpp`.** Public enum stays `1..7`; internal macOS mapping satisfies `pthread_attr_setschedparam` and `pthread_setschedparam`.
- **Replace the deprecated unnamed POSIX semaphore (`sem_t`) on macOS with a `pthread_cond_t` + counter implementation.** This fixes the `Take(timeout_ms)` race against `Give()` that causes `MessageBuffer` and `QueueBlocking` failures, and removes the deprecation-warning pragma.
- **Preserve Linux behavior exactly.** Linux code paths use `sem_t`, `sem_timedwait`, and raw enum priorities unchanged.
- **Do not change `Mutex` timeout helper.** The 1 ms polling `TimedLock()` is acceptable for the PC/test port and is not implicated in current failures.

### Claude's discretion

- Exact linear mapping for priorities: use the full valid macOS `SCHED_RR` range `[min, max]` with `kIdle=1 → min`, `kRealTime=7 → max`, and linear interpolation for the intermediate enum values.
- Semaphore max-count semantics: the macOS condition-variable backend must enforce `max_count` in `Give()` exactly as the Linux `sem_t` backend does.
- `CriticalSection` uses a recursive mutex inside `paraos_critical.hpp`; the semaphore backend is not used by critical sections, so replacing semaphores on macOS does not affect critical-section behavior.

## Canonical References

- `.planning/REQUIREMENTS.md` — PAR-01, PAR-02
- `.planning/ROADMAP.md` — Phase 2 scope
- `.planning/phases/phase-01/phase-01-SUMMARY.md` — what Phase 1 built
- `.planning/phases/phase-01/RESEARCH.md` — Phase 1 API audit and compatibility strategy
- `.planning/phases/02-verify-behavior-parity/02-RESEARCH.md` — this phase's research
- `paraos_thread_common.hpp` — public `ThreadPriority` enum
- `port_unix/paraos_thread.hpp` — Unix thread implementation
- `port_unix/paraos_semaphore.hpp` — Unix semaphore implementation
- `containers/paraos_queue_blocking.hpp` — blocking queue consumer
- `containers/paraos_message_buffer.hpp` — message buffer that wraps the queue

## Specific Ideas

- Add a private static helper `MapPriorityToSchedRange(ThreadPriority)` inside `port_unix/paraos_thread.hpp` under `#ifdef __APPLE__`.
- Add a private macOS-only `PthreadSemaphore` struct inside `port_unix/paraos_semaphore.hpp` with `pthread_mutex_t`, `pthread_cond_t`, `std::size_t count`, and `std::size_t max_count`.
- Keep `sem_t` storage only under `#ifdef __linux__`; on macOS replace it with the `PthreadSemaphore` struct.
- The `SemaphoreBinary::is_given_` flag logic remains unchanged; it sits on top of `SemaphoreBase::Take`/`Give`.

## Deferred Ideas

- macOS GitLab CI runner — deferred to v2 (CI-01).
- macOS-specific build instructions — deferred to v2 (CI-02).
- Performance benchmarking of the new semaphore backend — out of scope for behavior parity.

---

*Phase: 02-verify-behavior-parity*
*Context gathered: 2026-06-20*
