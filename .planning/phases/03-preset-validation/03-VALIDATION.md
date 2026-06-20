---
phase: 3
slug: preset-validation
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-06-20
---

# Phase 3 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + CMake presets |
| **Config file** | `CMakePresets.json` |
| **Quick run command** | `cmake --preset <name> && cmake --build build/<name> && ctest --test-dir build/<name> --output-on-failure --timeout 20` |
| **Full suite command** | Iterate quick command for every preset in `cmake --list-presets` |
| **Estimated runtime** | ~5–15 minutes depending on toolchain availability |

---

## Sampling Rate

- **After every task commit:** Re-run the quick command for the preset(s) touched by the task.
- **After every plan wave:** Re-run the full preset sweep.
- **Before `/gsd-verify-work`:** Full preset sweep must be complete and recorded.
- **Max feedback latency:** 5 minutes.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 03-01-01 | 01 | 1 | VAL-01 | — | N/A | CLI | `cmake --list-presets` | ✅ | ⬜ pending |
| 03-01-02 | 01 | 1 | VAL-02 | — | N/A | CLI | `cmake --build build/<preset>` per preset | ✅ | ⬜ pending |
| 03-01-03 | 01 | 1 | VAL-03 | — | N/A | CLI | `ctest --test-dir build/<preset>` per preset | ✅ | ⬜ pending |

---

## Wave 0 Requirements

- Existing infrastructure covers all phase requirements.

## Manual-Only Verifications

None — all verification is automated via CMake/CTest.

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 5 min
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
