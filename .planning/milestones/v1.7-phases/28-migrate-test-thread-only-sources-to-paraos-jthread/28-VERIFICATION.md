---
phase: 28
status: passed
verified_at: "2026-06-23"
---

# Phase 28 Verification: Migrate `test_thread_only_*` sources to `paraos::jthread`

## Summary

All four standalone `port_tests/test_thread_only_*.cpp` tests have been migrated from the legacy `paraos::Thread` API to the modern `paraos::jthread` API. The unified cross-platform lifecycle pattern (`start_scheduler()` → worker threads → `stopper` → `NotifySchedulerEnded()` → `IdleHook()` → `end_scheduler()`) has been applied consistently.

## Verification Results

| Criterion | Result | Evidence |
|-----------|--------|----------|
| `test_thread_only_global.cpp` uses `paraos::jthread` | ✅ Pass | Four global `paraos::jthread` objects with worker lambdas |
| `test_thread_only_static.cpp` uses `paraos::jthread` | ✅ Pass | Three `const static paraos::jthread` objects |
| `test_thread_only_stack.cpp` uses `std::vector<paraos::jthread>` | ✅ Pass | RAII vector, no `new`/`delete` of thread objects |
| `test_thread_only_stack_with_multiple_threads.cpp` uses RAII group | ✅ Pass | `std::vector<std::unique_ptr<MyThreadGroup>>` with three `paraos::jthread` and two `paraos::binary_semaphore` per group |
| No `paraos::Thread::StartScheduler/Exit/DeleteAll` | ✅ Pass | `rg` found none |
| No `std::_Exit()` platform branches | ✅ Pass | Removed all `#if defined(PARAOS_LIKE_FREERTOS) ... std::_Exit` blocks |
| PC build (`pc_debug_clang`) | ✅ Pass | Builds and four executables print `OK` |
| PC build (`pc_debug_gcc_clang_tidy`) | ✅ Pass | Builds with `CLANG_TIDY_ENABLE=true` and no new diagnostics on the four files |
| FreeRTOS build (`freertos_debug_clang`) | ✅ Pass | Configures and builds all four targets |
| FreeRTOS build (`freertos_debug_gcc`) | ✅ Pass | Configures and builds all four targets |

## Runtime Output (pc_debug_clang)

```
test_thread_only_global: OK
test_thread_only_static: OK
test_thread_only_stack: OK
test_thread_only_stack_with_multiple_threads: OK
```

## Notes

- `port_tests/CMakeLists.txt` was temporarily updated to `cxx_std_20` for the four targets so the migrated sources compile. The formal CMake update (clang-tidy registration, CTest timeout) is owned by Phase 29.
- `test_thread_only_stack_with_multiple_threads.cpp` deviates from the original literal "self-delete on completion" mechanism because `paraos::jthread` does not provide deferred self-deletion. The equivalent behavior is achieved by having the `stopper` thread explicitly clear the RAII container after all `thread_two_` workers have finished, so group destructors run while the scheduler is still active. The final `deleted_objects_cnt` still equals `EXPECTED_THREADS * 2`.
