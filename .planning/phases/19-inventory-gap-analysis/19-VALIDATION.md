---
phase: 19
slug: inventory-gap-analysis
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-06-22
---

# Phase 19 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

Phase 19 is a research and audit phase: it does not modify source code, so validation is based on static review and manual verification rather than automated tests.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | None — review-based phase |
| **Config file** | None |
| **Quick run command** | `cat .planning/phases/19-inventory-gap-analysis/19-RESEARCH.md` |
| **Full suite command** | Review `19-RESEARCH.md`, `19-CONTEXT.md`, and `19-PLAN.md` together |
| **Estimated runtime** | ~5 minutes peer review |

---

## Sampling Rate

- **After every task commit:** Review the updated inventory/gap section in `19-RESEARCH.md` or `19-CONTEXT.md`.
- **After every plan wave:** Run a full review of all phase artifacts against the success criteria in `ROADMAP.md`.
- **Before `/gsd-verify-work`:** All success criteria must be documented and peer-reviewed.
- **Max feedback latency:** 1 review cycle per task.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 19-01-01 | 19-01 | 1 | ANL-01 | — | N/A | review | `grep -RIn 'paraos::Thread' containers/tests/*.cpp` | ✅ | ⬜ pending |
| 19-01-02 | 19-01 | 1 | ANL-02 | — | N/A | review | `diff port_pc/paraos_jthread.hpp port_freertos/paraos_jthread.hpp` | ✅ | ⬜ pending |
| 19-01-03 | 19-01 | 1 | ANL-03 | — | N/A | review | `grep -RIn 'join_sem\|request_stop\|vTaskDelete' port_freertos/paraos_jthread.hpp` | ✅ | ⬜ pending |
| 19-01-04 | 19-01 | 1 | ANL-01 | — | N/A | review | `grep -RInE 'MutexGuard|SemaphoreBinary|SemaphoreCounting|paraos::mutex|counting_semaphore' containers/tests/*.cpp` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `19-RESEARCH.md` exists and contains a per-test inventory table.
- [ ] `19-RESEARCH.md` contains exact API signatures for both PC and FreeRTOS `paraos::jthread`.
- [ ] `19-RESEARCH.md` contains a ranked FreeRTOS gap / hardening list.
- [ ] `19-CONTEXT.md` is updated with the final inventory and API-difference tables.

*If none: "Existing infrastructure covers all phase requirements."*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Confirm no missed `paraos::Thread` patterns | ANL-01 | Requires human pattern matching across five test files | Read each test file and compare against the inventory table in `19-RESEARCH.md`. |
| Confirm Phase 21 scope is minimal | ANL-01 / SYNC-01 / SYNC-02 | Depends on design intent | Verify that none of the five tests use `paraos::Mutex` / `MutexGuard` / `paraos::Semaphore*`. |
| Confirm FreeRTOS gap list is feasible | ANL-03 | Requires architecture judgment | Review the ranked list in `19-RESEARCH.md` with the team and confirm each item fits Phase 22 scope. |

---

## Validation Sign-Off

- [ ] All tasks have a review-based verify command or Wave 0 dependency.
- [ ] Sampling continuity: no 3 consecutive tasks without documented verification.
- [ ] Wave 0 covers all MISSING references.
- [ ] No watch-mode flags.
- [ ] Feedback latency < 1 review cycle per task.
- [ ] `nyquist_compliant: true` set in frontmatter.

**Approval:** pending
