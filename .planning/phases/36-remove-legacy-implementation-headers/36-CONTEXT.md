# Phase 36: Remove legacy implementation headers - Context

**Gathered:** 2026-06-24
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped) — pure infrastructure cleanup

<domain>
## Phase Boundary

Delete legacy `paraos::Thread`, `paraos::Mutex`, and `paraos::Semaphore*` implementation headers from all three ports while preserving `ThreadAttr`, `ThreadPriority`, and other shared definitions in `paraos_thread_common.hpp` that `paraos::jthread` still needs.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — pure infrastructure cleanup phase. Use ROADMAP phase goal, success criteria, and codebase conventions to guide decisions.

### Cleanup scope
- Remove `port_unix/paraos_thread.hpp`, `port_unix/paraos_mutex.hpp`, `port_unix/paraos_semaphore.hpp`.
- Remove `port_win/paraos_thread.hpp`, `port_win/paraos_mutex.hpp`, `port_win/paraos_semaphore.hpp`.
- Remove `port_freertos/paraos_thread.hpp`, `port_freertos/paraos_mutex.hpp`, `port_freertos/paraos_semaphore.hpp`.
- Keep `paraos_thread_common.hpp` with `ThreadAttr`, `ThreadPriority`, `thread_delegate_type`.
- Remove obsolete `dtor_callback` and `run_` fields from `ThreadAttr` if no consumers remain.
- Rewrite or remove `paraos_mutex_raii.hpp` to use `std::lock_guard<paraos::mutex>` / `std::unique_lock<paraos::mutex>` semantics.

</decisions>

<canonical_refs>
## Canonical References

### Project-level requirements
- `.planning/REQUIREMENTS.md` — Milestone v1.9 requirements REM-01 through REM-05.
- `.planning/ROADMAP.md` — Phase 36 success criteria.

### Code conventions
- `AGENTS.md` — Commit style, coding standards, and build/test procedures.
- `.clang-tidy` — Static analysis rules that must remain clean after changes.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `paraos_thread_common.hpp` already isolates shared thread attributes.
- `paraos_jthread.hpp` in `port_pc/`, `port_unix/`, `port_win/`, `port_freertos/` replaces legacy `paraos::Thread`.
- `paraos_mutex_std.hpp` and `paraos_semaphore_std.hpp` provide std-like replacements.

### Established Patterns
- Port-specific headers are selected by `CMakeLists.txt` via `PARAOS_LIKE_UNIX` / `PARAOS_LIKE_WINAPI` / `PARAOS_LIKE_FREERTOS`.
- Legacy headers are included directly by consumers in `containers/`, `port_tests/`, and `port_unix/paraos_critical.hpp`.

### Integration Points
- `CMakeLists.txt` glob/source lists may reference legacy headers; update if needed.
- Any remaining `#include "paraos_thread.hpp"`, `#include "paraos_mutex.hpp"`, `#include "paraos_semaphore.hpp"`, `#include "paraos_mutex_raii.hpp"` must be rerouted or removed.

</code_context>

<specifics>
## Specific Ideas

No specific requirements — pure infrastructure cleanup. Preserve public `ThreadAttr`-based attribute structs in `extra/` that inherit from `paraos::ThreadAttr`.

</specifics>

<deferred>
## Deferred Ideas

None — cleanup phase.

</deferred>

---

*Phase: 36-remove-legacy-implementation-headers*
*Context gathered: 2026-06-24*
