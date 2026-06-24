# Phase 32: Migrate `ThreadSequence` to `paraos::jthread` - Plan

**Phase:** 32
**Goal:** `extra/paraos_thread_sequence.hpp` uses `paraos::jthread` internally while preserving its public API.
**Strategy:** Direct refactor of internal thread wrapper from legacy `paraos::Thread` to `paraos::jthread`.

## Plan

### 1. Refactor `extra/paraos_thread_sequence.hpp`

- Replace `#include "paraos_thread.hpp"` with `#include "paraos_jthread.hpp"`.
- Replace member `paraos::Thread thread_` with `paraos::jthread thread_` and reorder so `thread_` is declared after members it uses (`new_cycle_ready_sem_`, `timer_controller_`, `period_in_us_`, `nticks_`).
- Change `Run()` signature to accept `const paraos::stop_token& token` and loop while `!token.stop_requested()`:
  - Wait on `new_cycle_ready_sem_.Take(paraos::max_delay)`.
  - Call `timer_controller_.tick(nticks_)` and update `nticks_`.
  - Check `token.stop_requested()` at loop top.
- Update `IThreadSequence` constructor:
  - Initialize `thread_` with a callable that loops: `while (!token.stop_requested()) { Run(token); }`.
  - Use `static_cast<const paraos::ThreadAttr&>(attr)` to select the correct jthread constructor.
  - Remove `thread_.RegisterDelegate(...)` call.
  - Preserve `thread_start_flag` parameter for API compatibility (passed through, jthread blocks on scheduler gate).
- Update `Finish(bool is_dynamic)`:
  - `(void)thread_.request_stop();`
  - Call `NotifyGive(false)` to unblock `Run()`.
  - `thread_.join();`
  - Remove `is_dynamic` deferred-delete logic (ignore the parameter).
- Keep all public methods (`Register`, `Unregister`, `SetFreq`, `NotifyGive`, `GetMainFreq`, `GiveRegisteredDelegatesNumb`) unchanged in signature.
- Keep `IThreadSequenceAttr` / `ThreadSequenceAttr` as-is.

### 2. Update `extra/tests/test_paraos_thread_sequence.cpp`

- Remove `paraos::Thread` usage (`check_test_complete_and_exit`, `Thread::StartScheduler`, `Thread::DeleteAll`, `Thread::Exit`, `Thread::DelayMs`).
- Use a `paraos::jthread` stopper that waits for `is_test_complete` (use `std::atomic<bool>`) and then signals completion.
- After `thread_seq_ptr->NotifyGive(false)` is called to drive the sequence, call `paraos::jthread::start_scheduler()`.
- Wait for completion, then call `thread_seq_ptr->Finish(false)` and `paraos::jthread::end_scheduler()`.
- Replace `paraos::Thread::DelayMs(...)` with `paraos::sleep_for(std::chrono::milliseconds(...))`.
- Keep existing assertions on `gyracc_call_cnt`, `mag_call_cnt`, `baro_call_cnt`.

### 3. Verify

- Configure and build `pc_debug_clang` preset.
- Run `test_paraos_thread_sequence` standalone test.
- Run `[PARAOS EXTRA]` GoogleTest suite.

## Success Criteria Verification

- [ ] `IThreadSequence` owns a `paraos::jthread` member instead of `paraos::Thread`.
- [ ] The timer tick loop (`Run()`) runs inside the jthread callable and observes `stop_token`.
- [ ] `Finish()` requests stop and joins without relying on `Finished()`/`Base*` deferred deletion.
- [ ] `NotifyGive()` continues to wake the sequence thread.
- [ ] `test_paraos_thread_sequence.cpp` standalone test passes.
- [ ] No regressions in `[PARAOS EXTRA]` GoogleTest suite.
