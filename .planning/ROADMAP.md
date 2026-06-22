# Roadmap: PARAOS

## Milestones

- ✅ **v1.0 macOS Support** — Phases 1-4 (shipped earlier)
- ✅ **v1.1 Static Analysis Cleanup** — Phases 5-8 (shipped 2026-06-20) — see `.planning/milestones/v1.1-ROADMAP.md`
- ✅ **v1.2 std::jthread-style Thread API** — Phases 9-12 (shipped 2026-06-20) — see `.planning/milestones/v1.2-ROADMAP.md`
- ✅ **v1.3 std::mutex-style Mutex API** — Phases 13-15 (shipped 2026-06-22) — see `.planning/milestones/v1.3-ROADMAP.md`
- ✅ **v1.4 std::semaphore-style Semaphore API** — Phases 16-18 (shipped 2026-06-22) — see `.planning/milestones/v1.4-ROADMAP.md`
- ✅ **v1.5 Modernize container tests on std-like primitives** — Phases 19-23 (shipped 2026-06-22) — see `.planning/milestones/v1.5-ROADMAP.md`
- 🚧 **v1.6 Next** — TBD (in planning)

## Phases

<details>
<summary>✅ v1.0 macOS Support (Phases 1-4) — SHIPPED</summary>

- [x] Phase 1: macOS port foundation (1/1 plan) — completed
- [x] Phase 2: macOS thread/mutex/semaphore parity (1/1 plan) — completed
- [x] Phase 3: macOS timer and socket porting (1/1 plan) — completed
- [x] Phase 4: macOS build and test integration (1/1 plan) — completed

</details>

<details>
<summary>✅ v1.1 Static Analysis Cleanup (Phases 5-8) — SHIPPED 2026-06-20</summary>

- [x] Phase 5: Reproduce & Classify clang-tidy warnings (1/1 plan) — completed 2026-06-20
- [x] Phase 6: Fix Core, Headers & port_unix (1/1 plan) — completed 2026-06-20
- [x] Phase 7: Fix Tests, Examples & Document Suppressions (1/1 plan) — completed 2026-06-20
- [x] Phase 8: Regression Guard (1/1 plan) — completed 2026-06-20

</details>

<details>
<summary>✅ v1.2 std::jthread-style Thread API (Phases 9-12) — SHIPPED 2026-06-20</summary>

- [x] Phase 9: PC jthread implementation (1/1 plan) — completed 2026-06-20
- [x] Phase 10: FreeRTOS jthread implementation (1/1 plan) — completed 2026-06-20
- [x] Phase 11: Thread attributes integration (1/1 plan) — completed 2026-06-20
- [x] Phase 12: Build, tests and static analysis (1/1 plan) — completed 2026-06-20

</details>

<details>
<summary>✅ v1.3 std::mutex-style Mutex API (Phases 13-15) — SHIPPED 2026-06-22</summary>

- [x] Phase 13: PC mutex implementation (1/1 plan) — completed 2026-06-22
- [x] Phase 14: FreeRTOS mutex implementation (1/1 plan) — completed 2026-06-22
- [x] Phase 15: Build, tests and static analysis (1/1 plan) — completed 2026-06-22

</details>

<details>
<summary>✅ v1.4 std::semaphore-style Semaphore API (Phases 16-18) — SHIPPED 2026-06-22</summary>

- [x] Phase 16: PC counting_semaphore implementation (1/1 plan) — completed 2026-06-22
- [x] Phase 17: FreeRTOS counting_semaphore implementation (1/1 plan) — completed 2026-06-22
- [x] Phase 18: Build, tests and static analysis (1/1 plan) — completed 2026-06-22

</details>

<details>
<summary>✅ v1.5 Modernize container tests on std-like primitives (Phases 19-23) — SHIPPED 2026-06-22</summary>

- [x] Phase 19: Inventory & gap analysis (1/1 plan) — completed 2026-06-22
- [x] Phase 20: Migrate thread primitives in container tests (1/1 plan) — completed 2026-06-22
- [x] Phase 21: Migrate synchronization primitives in container tests (1/1 plan) — completed 2026-06-22
- [x] Phase 22: FreeRTOS std-like primitives hardening (1/1 plan) — completed 2026-06-22
- [x] Phase 23: Build, tests and static analysis (1/1 plan) — completed 2026-06-22

</details>

### 🚧 v1.6 Next (In Planning)

- [ ] Phase 24: TBD

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
| ----- | --------- | -------------- | ------ | --------- |
| 1-4. macOS Support | v1.0 | 4/4 | Complete | earlier |
| 5. Reproduce & Classify clang-tidy warnings | v1.1 | 1/1 | Complete | 2026-06-20 |
| 6. Fix Core, Headers & port_unix | v1.1 | 1/1 | Complete | 2026-06-20 |
| 7. Fix Tests, Examples & Document Suppressions | v1.1 | 1/1 | Complete | 2026-06-20 |
| 8. Regression Guard | v1.1 | 1/1 | Complete | 2026-06-20 |
| 9. PC jthread implementation | v1.2 | 1/1 | Complete | 2026-06-20 |
| 10. FreeRTOS jthread implementation | v1.2 | 1/1 | Complete | 2026-06-20 |
| 11. Thread attributes integration | v1.2 | 1/1 | Complete | 2026-06-20 |
| 12. Build, tests and static analysis | v1.2 | 1/1 | Complete | 2026-06-20 |
| 13. PC mutex implementation | v1.3 | 1/1 | Complete | 2026-06-22 |
| 14. FreeRTOS mutex implementation | v1.3 | 1/1 | Complete | 2026-06-22 |
| 15. Build, tests and static analysis | v1.3 | 1/1 | Complete | 2026-06-22 |
| 16. PC counting_semaphore implementation | v1.4 | 1/1 | Complete | 2026-06-22 |
| 17. FreeRTOS counting_semaphore implementation | v1.4 | 1/1 | Complete | 2026-06-22 |
| 18. Build, tests and static analysis | v1.4 | 1/1 | Complete | 2026-06-22 |
| 19. Inventory & gap analysis | v1.5 | 1/1 | Complete | 2026-06-22 |
| 20. Migrate thread primitives in container tests | v1.5 | 1/1 | Complete | 2026-06-22 |
| 21. Migrate synchronization primitives in container tests | v1.5 | 1/1 | Complete | 2026-06-22 |
| 22. FreeRTOS std-like primitives hardening | v1.5 | 1/1 | Complete | 2026-06-22 |
| 23. Build, tests and static analysis | v1.5 | 1/1 | Complete | 2026-06-22 |
| 24. TBD | v1.6 | 0/0 | Not started | - |
