# Roadmap: PARAOS

## Milestones

- ✅ **v1.0 macOS Support** — Phases 1-4 (shipped earlier)
- ✅ **v1.1 Static Analysis Cleanup** — Phases 5-8 (shipped 2026-06-20) — see `.planning/milestones/v1.1-ROADMAP.md`
- 🚧 **v1.2 CI & Documentation for macOS** — добавление macOS GitLab CI и документации (planned)

## Phases

<details>
<summary>✅ v1.1 Static Analysis Cleanup (Phases 5-8) — SHIPPED 2026-06-20</summary>

- [x] Phase 5: Reproduce & Classify clang-tidy warnings (1/1 plan) — completed 2026-06-20
- [x] Phase 6: Fix Core, Headers & port_unix (1/1 plan) — completed 2026-06-20
- [x] Phase 7: Fix Tests, Examples & Document Suppressions (1/1 plan) — completed 2026-06-20
- [x] Phase 8: Regression Guard (1/1 plan) — completed 2026-06-20

</details>

### 🚧 v1.2 CI & Documentation for macOS (Planned)

- [ ] Phase 9: macOS GitLab CI pipeline (2 plans)
  - [ ] 09-01: Добавить GitLab CI job для macOS-раннера (CI-01)
  - [ ] 09-02: Добавить GitLab CI job для `*_clang_tidy` пресетов на macOS (CI-02)
- [ ] Phase 10: macOS Build Documentation (2 plans)
  - [ ] 10-01: Документировать macOS-специфичные инструкции по сборке в `README.md` (DOCS-01)
  - [ ] 10-02: Задокументировать политику статического анализа и правила суппрессий (DOCS-02)

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
| ----- | --------- | -------------- | ------ | --------- |
| 5. Reproduce & Classify clang-tidy warnings | v1.1 | 1/1 | Complete | 2026-06-20 |
| 6. Fix Core, Headers & port_unix | v1.1 | 1/1 | Complete | 2026-06-20 |
| 7. Fix Tests, Examples & Document Suppressions | v1.1 | 1/1 | Complete | 2026-06-20 |
| 8. Regression Guard | v1.1 | 1/1 | Complete | 2026-06-20 |
| 9. macOS GitLab CI pipeline | v1.2 | 0/2 | Planned | - |
| 10. macOS Build Documentation | v1.2 | 0/2 | Planned | - |
