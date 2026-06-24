# Phase 37: Migrate internal consumers to std-like primitives - Context

**Gathered:** 2026-06-24
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped) — mechanical migration

<domain>
## Phase Boundary

Update remaining internal code that depends on legacy `paraos::Thread`, `paraos::Mutex`, or `paraos::Semaphore*` to use the std-like replacements (`paraos::jthread`, `paraos::mutex`, `paraos::*_semaphore`).

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — mechanical migration phase. Use ROADMAP phase goal, success criteria, and codebase conventions to guide decisions.

### Migration targets
- `containers/paraos_message_buffer.hpp` and `containers/paraos_queue_blocking.hpp` use `paraos::mutex` and `paraos::*_semaphore`.
- `port_unix/paraos_critical.hpp` uses a recursive mutex abstraction and no longer references `MutexBase`.
- `port_unix/paraos_socket_udp.hpp` and `port_win/paraos_socket_udp.hpp` use `paraos::jthread` / `paraos::sleep_for` instead of `paraos::Thread::DelayMs`.

### Recursive mutex for critical section
`paraos::CriticalSection` is inherently recursive (nested enter/exit via ETL macros and trace macros). Because `std::mutex` / `paraos::mutex` is non-recursive, introduce `paraos::recursive_mutex` as a std-like primitive for Unix/PC/Windows and FreeRTOS. This keeps the public API aligned with the existing `paraos::mutex` primitive while preserving recursive semantics.

</decisions>

<canonical_refs>
## Canonical References

### Project-level requirements
- `.planning/REQUIREMENTS.md` — Milestone v1.9 requirements MIG-01 through MIG-04.
- `.planning/ROADMAP.md` — Phase 37 success criteria.

### Code conventions
- `AGENTS.md` — Commit style, coding standards, and build/test procedures.
- `.clang-tidy` — Static analysis rules that must remain clean after changes.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `paraos::jthread`, `paraos::mutex`, and `paraos::counting_semaphore` / `paraos::binary_semaphore` already implemented across all ports.
- `paraos_mutex_raii.hpp` will be rewritten in Phase 36 to wrap std-like locks.

### Established Patterns
- `containers/paraos_queue_blocking.hpp` currently uses `MutexGuard` and `SemaphoreBase`.
- `port_unix/paraos_critical.hpp` currently uses static `MutexRecursive`.
- `port_unix/paraos_socket_udp.hpp` and `port_win/paraos_socket_udp.hpp` use `paraos::Thread::DelayMs` for polling delays.

### Integration Points
- `paraos_message_buffer.hpp` and `paraos_queue_blocking.hpp` are included by container tests.
- `paraos_critical.hpp` is included by atomic bool, trace, and FreeRTOS legacy mutex/thread headers (which are removed in Phase 36).

</code_context>

<specifics>
## Specific Ideas

No specific requirements — mechanical migration. Preserve ISR-safety behavior where required by existing consumers.

</specifics>

<deferred>
## Deferred Ideas

None — migration phase.

</deferred>

---

*Phase: 37-migrate-internal-consumers-to-std-like-primitives*
*Context gathered: 2026-06-24*
