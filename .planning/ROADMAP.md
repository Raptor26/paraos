# Roadmap: PARAOS

## Milestones

- ✅ **v1.0 macOS Support** — Phases 1-4 (shipped earlier)
- ✅ **v1.1 Static Analysis Cleanup** — Phases 5-8 (shipped 2026-06-20) — see `.planning/milestones/v1.1-ROADMAP.md`
- ✅ **v1.1 Static Analysis Cleanup** — Phases 5-8 (shipped 2026-06-20) — see `.planning/milestones/v1.1-ROADMAP.md`
- 🚧 **v1.2 std::jthread-style Thread API** — добавление `paraos::jthread`, единого API для Windows/Unix/FreeRTOS (planned)

## Phases

<details>
<summary>✅ v1.1 Static Analysis Cleanup (Phases 5-8) — SHIPPED 2026-06-20</summary>

- [x] Phase 5: Reproduce & Classify clang-tidy warnings (1/1 plan) — completed 2026-06-20
- [x] Phase 6: Fix Core, Headers & port_unix (1/1 plan) — completed 2026-06-20
- [x] Phase 7: Fix Tests, Examples & Document Suppressions (1/1 plan) — completed 2026-06-20
- [x] Phase 8: Regression Guard (1/1 plan) — completed 2026-06-20

</details>

### 🚧 v1.2 std::jthread-style Thread API (Planned)

- [ ] Phase 9: PC jthread implementation (1 plan)
  - [ ] 09-01: Реализовать `paraos::jthread` для Windows и Unix поверх `std::jthread` (JTHREAD-01..06, PORT-01..03)
- [ ] Phase 10: FreeRTOS jthread implementation (1 plan)
  - [ ] 10-01: Реализовать `paraos::jthread` поверх FreeRTOS API с поддерж capturing lambdas (JTHREAD-01..06, PORT-04)
- [ ] Phase 11: Thread attributes integration (1 plan)
  - [ ] 11-01: Поддержать `ThreadAttr` (приоритет, стек, имя) в `jthread` для всех портов (JTHREAD-07..10)
- [ ] Phase 12: Build, tests and static analysis (1 plan)
  - [ ] 12-01: Обновить CMake до C++20, добавить `test_jthread_basic.cpp`, прогнать все пресеты и tidy (BUILD-01..02, TEST-01..05)

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
| ----- | --------- | -------------- | ------ | --------- |
| 5. Reproduce & Classify clang-tidy warnings | v1.1 | 1/1 | Complete | 2026-06-20 |
| 6. Fix Core, Headers & port_unix | v1.1 | 1/1 | Complete | 2026-06-20 |
| 7. Fix Tests, Examples & Document Suppressions | v1.1 | 1/1 | Complete | 2026-06-20 |
| 8. Regression Guard | v1.1 | 1/1 | Complete | 2026-06-20 |
| 9. PC jthread implementation | v1.2 | 0/1 | Planned | - |
| 10. FreeRTOS jthread implementation | v1.2 | 0/1 | Planned | - |
| 11. Thread attributes integration | v1.2 | 0/1 | Planned | - |
| 12. Build, tests and static analysis | v1.2 | 0/1 | Planned | - |
