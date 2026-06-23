---
phase: 34
status: complete
requirements_completed:
  - MIG-04
  - BUILD-01
  - BUILD-02
one_liner: Migrated extra standalone tests to jthread, required cxx_std_20, and attached CXX_CLANG_TIDY.
---

# Phase 34 Summary

Updated `extra/tests/CMakeLists.txt` to require `cxx_std_20` for all targets and attach `CXX_CLANG_TIDY` to standalone executables. Migrated the three standalone thread test files to `paraos::jthread`, `start_scheduler()`, and `end_scheduler()`.
