# Phase 7: Fix Tests, Examples & Document Suppressions - Context

**Gathered:** 2026-06-20
**Status:** Ready for planning
**Mode:** Auto-generated (autonomous — Phase 7 follows directly from Phase 6)

<domain>
## Phase Boundary

Fix or document-suppress all remaining clang-tidy warnings in tests, examples, and other ancillary files:
- `port_tests/*.cpp` (executable examples and unit tests)
- `containers/tests/*.cpp`
- `extra/tests/*.cpp`
- `port_unix/tests/*.cpp`

Also document every suppression rationale as required by SUPP-01, and ensure SUPP-02 is satisfied (no whole-category `.clang-tidy` disables). Test logic and assertions must be preserved; only style/static-analysis issues may change.

</domain>

<decisions>
## Implementation Decisions

### Source of Truth
- `.planning/phases/phase-05/WARNINGS.md` is the authoritative input.
- Remaining warnings after Phase 6 are all in test/example `.cpp` files.

### Fix Strategy by Check
- `misc-use-internal-linkage`: Move file-local structs/classes into an anonymous namespace.
- `llvm-prefer-static-over-anonymous-namespace`: Add `static` to file-local helper functions already in anonymous namespaces, OR move them out of anonymous namespaces and mark `static`. Prefer `static` for functions to match LLVM style.
- `misc-override-with-different-visibility`: Change `SetUp()` visibility in `test_runtime_profiler.cpp` from `public` to `protected`.
- `llvm-use-ranges`: Replace `std::find(begin, end, value)` with `std::ranges::find(begin, end, value)` (C++17 with `<algorithm>` + `<iterator>`; requires `std::ranges` available — on C++17 this may need `<experimental/ranges>` or be unsupported. If unavailable, use an inline `// NOLINT(llvm-use-ranges)` suppression with rationale).
- `readability-redundant-parentheses`: Remove redundant parentheses.
- `readability-use-concise-preprocessor-directives`: Replace `#if defined(X)` with `#ifdef X` in `.cpp` files.

### Suppression Documentation
- Every inline suppression must include a rationale comment.
- Collect all suppression rationales in a `SUPPRESSIONS.md` file in `.planning/phases/phase-07/` for SUPP-01.
- No `.clang-tidy` whole-category disables.

### Scope Boundaries
- Do not modify `port_win/` or `port_freertos/` sources.
- Do not change public API signatures.
- Do not modify library headers (Phase 6 scope).
- Do not modify `etl/`, `leaf/`, `GSL/`, `lwrb/`, or `FreeRTOS-Kernel/` vendored code.

### Claude's Discretion
- Choice between anonymous-namespace move vs. `static` for internal linkage warnings.
- Whether to use `std::ranges::find` or inline suppression if ranges support is unclear.
- Exact wording of suppression rationale comments.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- Test helpers are duplicated across multiple container test files (`Producer`, `Consumer`, `ExitFromTest`, etc.).
- `port_tests/test_runtime_profiler.cpp` uses a `Profiler` GoogleTest fixture with `SetUp()` declared `public`.
- `port_tests/example_socket_udp.cpp` has redundant parentheses around boolean variables in an `if` condition.

### Established Patterns
- Tests use anonymous namespaces for file-local helpers.
- Container tests share naming conventions (`Producer`, `Consumer`, `ExitFromTest`, `CheckIfTestSuccessfullyComplete`).
- `PARAOS_LIKE_FREERTOS` guards appear in several test `.cpp` files.

### Integration Points
- Test files are compiled by both PC and FreeRTOS presets.
- Any change must preserve test behavior and assertions.

</code_context>

<specifics>
## Specific Ideas

No specific requirements — follow Phase 5 classification log and project conventions.

</specifics>

<deferred>
## Deferred Ideas

- Regression guard / non-tidy preset verification → Phase 8.
- macOS CI runner / documentation → v2 (CI-01/CI-02/DOCS-01/DOCS-02).

</deferred>
