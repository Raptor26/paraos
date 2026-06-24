# Phase 33: Migrate `CooperativeScheduling` to `paraos::jthread` - Plan

**Phase:** 33
**Goal:** `extra/paraos_thread_cooperative_scheduling.hpp` uses `paraos::jthread` internally while preserving its public API.
**Strategy:** Direct refactor of internal thread wrapper from legacy `paraos::Thread` to `paraos::jthread`.

## Plan

### 1. Refactor `extra/paraos_thread_cooperative_scheduling.hpp`

- Replace `#include "paraos_thread.hpp"` with `#include "paraos_jthread.hpp"`.
- Replace member `paraos::Thread thread_` with `paraos::jthread thread_` and reorder so `thread_` is declared after `scheduler_` and `new_cycle_ready_sem_`.
- Change `Run()` signature to accept `const paraos::stop_token& token` and check `token.stop_requested()` before calling `scheduler_.start()`.
- Update `ICooperativeScheduling` constructor:
  - Initialize `thread_` with a callable that calls `Run(token)` once (or loops until stop requested).
  - Use `static_cast<const paraos::ThreadAttr&>(attr)` to select the correct jthread constructor.
  - Remove `thread_.RegisterDelegate(...)` call.
  - Preserve `thread_start_flag` parameter for API compatibility.
- Update `Finish(bool is_dynamic)`:
  - `(void)is_dynamic;`
  - `(void)thread_.request_stop();`
  - `scheduler_.exit_scheduler();`
  - `NotifyGive();` to unblock `Idle()` if waiting.
  - `thread_.join();`
  - Remove `is_dynamic` deferred-delete logic.
- Update destructor `~ICooperativeScheduling()`:
  - Keep calling `Finish();` but ensure it is safe when the scheduler thread is the caller. Since `Finish()` now joins, the destructor should only be called from a thread other than the scheduler thread. Document this assumption.
- Keep public methods `NotifyGive`, `AddTask`, `SetIdleCallback`, `GetScheduler` unchanged.
- Replace `paraos::Thread::DelayMs(coop_scheduler_delay_ms)` inside `Run()` with `paraos::sleep_for(std::chrono::milliseconds(coop_scheduler_delay_ms))`.

### 2. Update `extra/tests/test_paraos_cooperative_scheduling_thread.cpp`

- Remove `paraos::Thread` usage (`check_test_complete_and_exit`, `Thread::StartScheduler`, `Thread::DeleteAll`, `Thread::Exit`, `Thread::DelayMs`).
- Use `std::atomic<bool>` for `is_test_complete`.
- Use a `paraos::jthread` stopper that waits for `is_test_complete` and then calls `cooperative_scheduler.Finish(false)` and `paraos::jthread::end_scheduler()`.
- After setting up tasks and idle callback, call `paraos::jthread::start_scheduler()`.
- Wait for completion (condition variable or stopper join).
- Keep existing task priorities and assertions.

### 3. Verify

- Configure and build `pc_debug_clang` preset.
- Run `test_paraos_cooperative_scheduling_thread` standalone test.
- Run `[PARAOS EXTRA]` GoogleTest suite.
- Run Phase 31/32 standalone tests to ensure no regressions.

## Success Criteria Verification

- [ ] `ICooperativeScheduling` owns a `paraos::jthread` member instead of `paraos::Thread`.
- [ ] The scheduler loop (`Run()`) runs inside the jthread callable and observes `stop_token`.
- [ ] `Finish()` exits the scheduler, requests stop, and joins the thread.
- [ ] `AddTask()` and `SetIdleCallback()` keep existing signatures.
- [ ] `test_paraos_cooperative_scheduling_thread.cpp` standalone test passes.
- [ ] No regressions in `[PARAOS EXTRA]` GoogleTest suite or earlier standalone tests.
