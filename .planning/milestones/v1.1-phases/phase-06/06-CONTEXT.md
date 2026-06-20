# Phase 6: Fix Core, Headers & port_unix - Context

**Gathered:** 2026-06-20
**Status:** Ready for planning
**Mode:** Auto-generated (autonomous — Phase 6 follows directly from Phase 5 classification)

<domain>
## Phase Boundary

Fix or document-suppress all clang-tidy warnings classified in `.planning/phases/phase-05/WARNINGS.md` that originate in:
- Public core headers (`paraos_*.hpp` / `paraos_*.h`)
- `port_unix/` sources
- `extra/` library headers that participate in the tidy builds

Public API signatures must remain unchanged. Linux-specific code paths under `__linux__` must stay semantically identical. macOS-specific code paths introduced in v1.0 must remain tidy-clean. The existing `.clang-tidy` check set must be preserved; only documented false-positive suppressions may be added.

</domain>

<decisions>
## Implementation Decisions

### Source of Truth
- `.planning/phases/phase-05/WARNINGS.md` is the authoritative input.
- Fixes should be prioritized by impact on the build: core headers first, then `port_unix/`, then `extra/` library code.

### True-Positive Fixes
- `readability-use-concise-preprocessor-directives`: replace `#if defined(X)` with `#ifdef X` and `#if !defined(X)` with `#ifndef X` where equivalent.
- `readability-redundant-parentheses`: remove redundant parentheses.

### False-Positive Suppressions
- `misc-multiple-inheritance` in `paraos_exceptions.hpp` is intentional — `paraos::exception` must be catchable as both `std::exception` and `etl::exception`. Suppress inline with `NOLINT` and a rationale comment.
- `misc-multiple-inheritance` in `extra/paraos_thread_cooperative_scheduling.hpp` is intentional — inherits interface + implementation. Suppress inline with rationale.

### Scope Boundaries
- Do not modify `port_win/` or `port_freertos/` sources.
- Do not change public API signatures.
- Do not modify `etl/`, `leaf/`, `GSL/`, `lwrb/`, or `FreeRTOS-Kernel/` vendored code.
- Test/example warnings are out of scope for this phase (Phase 7).

### Claude's Discretion
- Specific inline suppression style (`// NOLINT(...)` vs `// NOLINTNEXTLINE(...)`).
- Whether to refactor small adjacent issues opportunistically, provided they do not change API or behavior.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `paraos::exception` combines `std::exception` and `etl::exception` — this design is load-bearing for downstream error handling.
- `paraos_thread_common.hpp` contains platform-specific `ThreadPriority` enums guarded by `PARAOS_LIKE_*` macros.
- `paraos_config.hpp` defines `PARAOS_FORCEINLINE`, `PARAOS_DEPRECATED`, and compiler-detection macros.

### Established Patterns
- Platform isolation uses `PARAOS_LIKE_UNIX`, `PARAOS_LIKE_WINAPI`, `PARAOS_LIKE_FREERTOS` plus `__APPLE__` / `__linux__` where needed.
- Existing `NOLINT` markers appear in some files (e.g., `paraos_thread_common.hpp` already has `NOLINTBEGIN`/`NOLINTEND` around member init).

### Integration Points
- Headers are included across all ports and tests; core-header changes must compile cleanly on every platform.
- `port_unix/` sources are compiled into the `paraos` static library target for both PC and FreeRTOS POSIX simulator builds.

</code_context>

<specifics>
## Specific Ideas

No specific requirements — follow Phase 5 classification log and project conventions.

</specifics>

<deferred>
## Deferred Ideas

- Test/example tidy warnings → Phase 7.
- Regression guard / non-tidy preset verification → Phase 8.
- `.clang-tidy` check-set policy evolution → Phase 7 / post-milestone.

</deferred>
