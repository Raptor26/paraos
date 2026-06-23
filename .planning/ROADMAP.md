# Roadmap: PARAOS

## Milestones

- ✅ **v1.0 macOS Support** — Phases 1-4 (shipped earlier)
- ✅ **v1.1 Static Analysis Cleanup** — Phases 5-8 (shipped 2026-06-20) — see `.planning/milestones/v1.1-ROADMAP.md`
- ✅ **v1.2 std::jthread-style Thread API** — Phases 9-12 (shipped 2026-06-20) — see `.planning/milestones/v1.2-ROADMAP.md`
- ✅ **v1.3 std::mutex-style Mutex API** — Phases 13-15 (shipped 2026-06-22) — see `.planning/milestones/v1.3-ROADMAP.md`
- ✅ **v1.4 std::semaphore-style Semaphore API** — Phases 16-18 (shipped 2026-06-22) — see `.planning/milestones/v1.4-ROADMAP.md`
- ✅ **v1.5 Modernize container tests on std-like primitives** — Phases 19-23 (shipped 2026-06-22) — see `.planning/milestones/v1.5-ROADMAP.md`
- ✅ **v1.6 paraos::jthread scheduler control** — Phases 24-27 (shipped 2026-06-22) — see `.planning/milestones/v1.6-ROADMAP.md`
- ✅ **v1.7 Migrate `test_thread_only_*` to `paraos::jthread`** — Phases 28-30 (shipped 2026-06-23) — see `.planning/milestones/v1.7-ROADMAP.md`

## Phases

### ✅ v1.7 Migrate `test_thread_only_*` to `paraos::jthread` (SHIPPED 2026-06-23)

- [x] Phase 28: Migrate `test_thread_only_*` sources to `paraos::jthread` (1/1 plans) — completed 2026-06-23
- [x] Phase 29: Update `port_tests/CMakeLists.txt` for new tests (1/1 plans) — completed 2026-06-23
- [x] Phase 30: Runtime verification on macOS (1/1 plans) — completed 2026-06-23

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
| 19. Inventory & gap analysis | v1.5 | 1/1 | Complete | 2026-06-20 |
| 20. Migrate thread primitives in container tests | v1.5 | 1/1 | Complete | 2026-06-22 |
| 21. Migrate synchronization primitives in container tests | v1.5 | 1/1 | Complete | 2026-06-22 |
| 22. FreeRTOS std-like primitives hardening | v1.5 | 1/1 | Complete | 2026-06-22 |
| 23. Build, tests and static analysis | v1.5 | 1/1 | Complete | 2026-06-22 |
| 24. FreeRTOS scheduler API | v1.6 | 2/2 | Complete | 2026-06-22 |
| 25. PC scheduler state and gating | v1.6 | 3/3 | Complete | 2026-06-22 |
| 26. Test unification | v1.6 | 7/7 | Complete | 2026-06-22 |
| 27. Build and static analysis verification | v1.6 | 6/6 | Complete | 2026-06-22 |
| 28. Migrate `test_thread_only_*` sources to `paraos::jthread` | v1.7 | 1/1 | Complete | 2026-06-23 |
| 29. Update `port_tests/CMakeLists.txt` for new tests | v1.7 | 1/1 | Complete | 2026-06-23 |
| 30. Runtime verification on macOS | v1.7 | 1/1 | Complete | 2026-06-23 |
