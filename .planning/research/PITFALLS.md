# Research: Pitfalls — std::mutex-style mutex for PARAOS

**Milestone:** v1.3 std::mutex-style Mutex API
**Dimension:** Common mistakes when adding a std::mutex-compatible wrapper to an existing OSAL.

## Pitfall 1: ABI or API mismatch with std::mutex

- `std::lock_guard` only requires `lock()` and `unlock()`.
- `std::unique_lock` additionally requires `try_lock()`.
- Returning wrong type or adding extra parameters breaks compatibility.
- **Prevention:** keep signatures exactly `void lock()`, `bool try_lock()`, `void unlock()`.

## Pitfall 2: Mixing new `paraos::mutex` with existing `paraos::Mutex`

- They have different APIs (`lock()` vs `Lock(timeout_ms, is_isr)`).
- Migrating existing code is out of scope; accidentally using one in place of the other will not compile, which is good.
- **Prevention:** keep the two classes independent and do not add implicit conversions.

## Pitfall 3: FreeRTOS mutex creation failure

- `xSemaphoreCreateMutex()` can return `nullptr` if heap is exhausted.
- `std::mutex` constructor is `noexcept` in practice; throwing from constructor is allowed in C++ but complicates RAII.
- **Prevention:** use `ETL_ASSERT` or `PARAOS_CHECK_ASSERT` on `nullptr`, consistent with existing `paraos::Mutex`. Document that construction assumes sufficient heap.

## Pitfall 4: Move semantics

- `std::mutex` is non-copyable and non-movable in the standard.
- Making `paraos::mutex` movable could let users move a locked mutex, which is dangerous.
- **Prevention:** delete copy and move operations, matching `std::mutex`.

## Pitfall 5: Deadlock from recursive locking

- `std::mutex` is non-recursive; `lock()` twice from the same thread is undefined behavior.
- FreeRTOS `xSemaphoreCreateMutex()` creates a non-recursive mutex, matching this behavior.
- **Prevention:** use non-recursive FreeRTOS mutex; do not add recursive semantics.

## Pitfall 6: clang-tidy warnings

- `WarningsAsErrors: '*'` turns any new warning into a build failure.
- Common issues: missing `explicit`, non-`noexcept`, virtual destructors, implicit bool conversions.
- **Prevention:** mark constructors `noexcept` where possible, delete copy/move, keep methods simple, add `// NOLINT` only with rationale.

## Pitfall 7: Test hang on FreeRTOS POSIX simulator

- FreeRTOS runtime tests on macOS POSIX simulator may hang, as observed with `test_jthread_basic`.
- **Prevention:** design the test to terminate from inside the scheduler on FreeRTOS, similar to `test_jthread_basic.cpp`.

## Pitfall 8: Forgetting to register the test in CMake

- If not added to `add_test`, the binary builds but CTest ignores it.
- **Prevention:** add both `add_executable` and `add_test`, plus clang-tidy properties if needed.
