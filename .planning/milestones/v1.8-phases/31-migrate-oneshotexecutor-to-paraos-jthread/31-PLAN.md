# Phase 31: Migrate `OneShotExecutor` to `paraos::jthread` - Plan

**Phase:** 31
**Goal:** `extra/paraos_oneshot_executor.hpp` uses `paraos::jthread` internally while preserving its public API.
**Strategy:** Direct refactor of internal thread wrapper from legacy `paraos::Thread` to `paraos::jthread`.

## Plan

### 1. Refactor `extra/paraos_oneshot_executor.hpp`

- Replace member `paraos::Thread thread_` with `paraos::jthread thread_`.
- Remove `RegisterDelegate()` call from constructor; instead construct `jthread` with a callable that:
  - captures `this`,
  - receives `paraos::stop_token`,
  - loops calling the delegate-processing logic until `stop_requested()` is true.
- Keep `ExecuteDelegates()` as the per-iteration queue pop/execute helper, but change its visibility to `private` and make it `void` (no return value).
- Update `Finish()`:
  - Call `thread_.request_stop()` to signal the loop to exit.
  - Enqueue an empty delegate to unblock `queue_.Pop()`.
  - Call `thread_.join()` to wait for graceful shutdown.
- Preserve all public `EnqueueDelegate` overloads unchanged.
- Keep `IOneShotExecutorAttributes` / `OneShotExecutorAttributes` as-is (already derive from `ThreadAttr`).
- Preserve constructor `thread_start_flag` parameter for API compatibility; the jthread will be constructed always and block on the scheduler gate when the scheduler is not running. This matches the migrated test pattern.

### 2. Update `extra/tests/test_oneshot_executor_thread.cpp`

- Replace `paraos::Thread::StartScheduler()` / `paraos::Thread::DeleteAll()` / `paraos::Thread::Exit()` with the jthread scheduler lifecycle:
  - `paraos::jthread::start_scheduler()` to start execution.
  - Use `paraos::jthread::end_scheduler()` or RAII/exit logic instead of `Thread::DeleteAll()`/`Thread::Exit()`.
- Remove `paraos::Thread` includes/usages (`check_test_complete_and_exit` thread, etc.). Use a `paraos::jthread` for the watchdog/exit logic.
- Ensure `ExitFromTest` still signals completion, calls `oneshot_executor_ptr->Finish()`, and ends the scheduler cleanly.

### 3. Verify

- Configure and build `pc_debug_clang` preset.
- Run relevant tests:
  - `test_paraos_extra` (GoogleTest suite containing `test_oneshot_executor.cpp`).
  - `test_paraos_oneshot_executor` standalone executable.
- Check no new clang-tidy warnings in modified files.

## Risks

- `Finish()` may hang if called before scheduler starts because the jthread blocks on the scheduler gate. Mitigation: only call `Finish()` after `start_scheduler()`; tests already do this.
- `ExecuteDelegates()` is currently public; making it private is a breaking change for direct callers, but it is documented as internal and not part of the public API contract.

## Dependencies

- `paraos::jthread` implementation (completed in milestone v1.2).

## Success Criteria Verification

- [ ] `IOneShotExecutor` owns a `paraos::jthread` member instead of `paraos::Thread`.
- [ ] The delegate-processing loop runs inside the jthread callable.
- [ ] `Finish()` signals stop and joins the thread gracefully on PC.
- [ ] Public `EnqueueDelegate` overloads remain unchanged.
- [ ] `test_oneshot_executor.cpp` GoogleTest unit tests still pass with `thread_start_flag=false`.
- [ ] `test_oneshot_executor_thread.cpp` standalone test passes.
