# Phase 4: Regression Guard — Context

**Gathered:** 2026-06-20
**Status:** Ready for planning
**Source:** Phase 3 completion state and project requirements REG-01, REG-02, REG-03.

## Phase Boundary

Audit the full milestone diff to confirm that macOS changes do not regress
Linux, Windows, or FreeRTOS builds or semantics. Update public documentation if
needed. Produce a final summary and mark the milestone complete.

## Implementation Decisions

### Locked decisions

- Diff scope is bounded to the GSD milestone start commit (`0680965`).
- Only `port_unix/` files may contain macOS-specific code paths; all other
  changes must be justified and documented.
- GitLab CI presets are not modified because the changes do not affect Linux or
  Windows toolchains.
- README.md is the appropriate place for the macOS build note.

### Claude's discretion

- Decide whether any additional files need updates (CHANGELOG, deferred docs,
  etc.). Default is to keep changes minimal.
- Classify Phase 3 FreeRTOS debug failures as a documented platform limitation
  rather than a regression.

## Canonical References

- `.planning/REQUIREMENTS.md` — REG-01, REG-02, REG-03
- `.planning/ROADMAP.md` — Phase 4 scope
- `.planning/phases/03-preset-validation/03-SUMMARY.md` — Phase 3 results
- `README.md`
- `.gitlab-ci.yml`

## Specific Ideas

- Run `git diff <start> HEAD` and classify every changed file.
- Verify that all `__APPLE__` blocks are inside `port_unix/` except the two
  necessary enablements in `paraos_check.h` and `paraos_runtime_profiler.hpp`.
- Confirm that `__linux__` paths remain byte-for-byte unchanged.
- Update README.md with a short macOS note and AppleClang override hint.
- Write `04-SUMMARY.md` and update tracking files.

## Deferred Ideas

- Adding a macOS GitLab CI runner remains deferred to v2.
- Full macOS-specific build guide remains deferred to v2.

---
*Phase: 04-regression-guard*
*Context gathered: 2026-06-20*
