---
phase: 2
slug: verify-behavior-parity
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-06-20
---

# Phase 2 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + GoogleTest |
| **Config file** | `CMakePresets.json` |
| **Quick run command** | `ctest --test-dir build/pc_debug_clang -R '<test_name>' --output-on-failure` |
| **Full suite command** | `ctest --test-dir build/pc_debug_clang --output-on-failure --stop-on-failure --schedule-random --timeout 20` |
| **Estimated runtime** | ~5 seconds |

---

## Sampling Rate

- **After every task commit:** Run the task's affected CTest target(s) with `--output-on-failure`.
- **After every plan wave:** Run the full `pc_debug_clang` CTest suite with standard CI flags.
- **Before `/gsd-verify-work`:** Full suite must be green.
- **Max feedback latency:** 30 seconds.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 02-01-01 | 01 | 1 | PAR-01 | — | N/A | unit / build | `cmake --build build/pc_debug_clang` + `ctest -R test_thread_only_static` | ✅ | ⬜ pending |
| 02-01-02 | 01 | 1 | PAR-01 | — | N/A | unit / build | `cmake --build build/pc_debug_clang` + message-buffer tests | ✅ | ⬜ pending |
| 02-01-03 | 01 | 1 | PAR-01, PAR-02 | — | N/A | integration | Full `ctest --test-dir build/pc_debug_clang` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- Existing infrastructure covers all phase requirements.

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Priority scheduling under root | PAR-01 | Requires superuser privileges, cannot run in CI | `sudo ctest --test-dir build/pc_debug_clang -R test_thread_only_global` and inspect `SetPriority` return values |

*If root test is unavailable: document that non-root behavior is verified and root behavior is identical to Linux by code inspection.*

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30 s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
