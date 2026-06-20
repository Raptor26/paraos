# Testing Patterns

**Analysis Date:** 2026-06-20

## Test Framework

**Runner:**
- GoogleTest — `find_package(GTest REQUIRED)` in each test `CMakeLists.txt`.
- CTest — Used for discovery and execution via `enable_testing()` / `include(CTest)`.
- `gtest_discover_tests` is used in `containers/tests/`, `extra/tests/`, and `port_unix/tests/` to register individual `TEST`/`TEST_F` cases.

**Assertion Library:**
- GoogleTest built-in assertions (`EXPECT_TRUE`, `EXPECT_EQ`, `ASSERT_NO_THROW`, etc.).

**Run Commands:**

```bash
# Configure and build a preset
cmake --preset pc_debug_clang
cmake --build build/pc_debug_clang/

# Run all tests
ctest --test-dir build/pc_debug_clang/

# Run with CI-style flags
ctest --test-dir build/pc_debug_clang/ \
  --output-on-failure \
  --stop-on-failure \
  --schedule-random \
  --timeout 20

# Stress tests only
ctest --test-dir build/<preset> -L stress --repeat-until-fail 150 --timeout 20

# Memory check
ctest --test-dir build/pc_debug_gcc -T memcheck -j4
```

## Test File Organization

**Location:**
- `port_tests/test_*.cpp` — OSAL primitive tests.
- `port_tests/example_*.cpp` — Standalone runnable examples (not registered as CTest tests).
- `containers/tests/test_*.cpp` — Container unit tests.
- `extra/tests/test_*.cpp` — Extra helper tests.
- `port_unix/tests/test_paraos_utils.cpp` — Port-specific utility tests.

**Naming:**
- `test_<module>.cpp` for unit tests.
- `test_<module>_thread.cpp` for multithread variants.
- `test_<module>_mpmc.cpp`, `_mpsc.cpp`, `_spmc.cpp` for stress tests.
- `example_<feature>.cpp` for standalone executables.

**Structure:**
```
port_tests/
  test_mutex.cpp
  test_mutex_raii.cpp
  test_semaphore.cpp
  test_runtime_profiler.cpp
  test_thread_only_static.cpp
  example_timer.cpp
  example_socket_udp.cpp
  ...
containers/tests/
  test_queue_blocking.cpp
  test_message_buff.cpp
  test_multi_ringbuff.cpp
  test_queue_blocking_mpmc.cpp   # stress
  ...
extra/tests/
  test_oneshot_executor.cpp
  test_paraos_cooperative_scheduling.cpp
  test_paraos_thread_sequence.cpp
  ...
```

## Test Structure

**GoogleTest suites:**

```cpp
TEST_F(MutexTest, LockUnlock) {
  paraos::Mutex mutex;
  ASSERT_TRUE(mutex.Lock(100));
  EXPECT_TRUE(mutex.Unlock());
}
```

**Patterns:**
- `TEST_F` fixtures for shared setup (e.g., `MutexTest`).
- Assert/expect on boolean return values from OSAL APIs.
- Multithread tests spin producer/consumer threads and verify data integrity.

## Mocking

**Framework:**
- No dedicated mocking library. Helpers can be compiled with `PARAOS_USING_POLYMORPHIC_EXTRA=true` to add `virtual` to key methods (`Register`, `Unregister`, `SetFreq`, `NotifyGive`, `GiveRegisteredDelegatesNumb`, `GetMainFreq`, `Run`) for subclass mocking.

**Patterns:**
- Inherit from helper classes and override virtual methods in test code.
- `PARAOS_POLYMORPHIC_EXTRA` macro toggles `virtual` keyword.

## Fixtures and Factories

- Tests typically construct objects inline.
- Some tests use `paraos::testing::Semaphore` helper (`paraos_testing_semaphore.hpp`) for deterministic synchronization.
- ETL delegate creation patterns are exercised directly in tests.

## Coverage

**Requirements:**
- No enforced coverage target in the current configuration.
- Optional GCC + lcov coverage via `CODE_COVERAGE=true` and `cmake/CodeCoverage.cmake`.

**Run Coverage:**
```bash
ctest --test-dir build/pc_debug_gcc -T Test -T Coverage
```

## Test Types

**Unit Tests:**
- Mutex, semaphore, runtime profiler, version, ISR helpers, critical section.
- Fast, single-threaded, deterministic.

**Integration / OSAL Tests:**
- Thread lifecycle tests (`test_thread_only_static`, `_global`, `_stack`, `_with_multiple_threads`).
- Timer examples and socket examples.

**Stress Tests:**
- Label: `stress` in CMake (`set_tests_properties(... PROPERTIES LABELS stress)`).
- Located in `containers/tests/` (`test_message_multithread_many_producer_many_consumers`, `test_multi_ringbuff_mpmc`, `test_queue_blocking_*`) and `extra/tests/` (`test_paraos_thread_sequence`, `test_paraos_cooperative_scheduling_thread`, `test_paraos_oneshot_executor`).
- Repeated 120–300 times in CI (`--repeat-until-fail`).

**Memory Checks:**
- Valgrind memcheck via CTest for `pc_debug_gcc`.

## CI Integration

- `.gitlab-ci.yml` runs `pybuilder/pycmakebuilder.py` with include/exclude regex to build subsets of presets and run tests.
- Stress stages use `-L [stress]` and `--repeat-until-fail`.
- FreeRTOS stages run tests single-threaded (`-j1`).

## Common Patterns

**Error Testing:**
```cpp
EXPECT_THROW({
  // code expected to throw paraos::thread_not_created_exception
}, paraos::thread_not_created_exception);
```

**Multithread Stress:**
```cpp
std::thread producer([&]() {
  for (int i = 0; i < kCount; ++i) {
    while (!queue.TryPush(i)) {}
  }
});
std::thread consumer([&]() {
  for (int i = 0; i < kCount; ++i) {
    auto v = queue.Pop();
    ASSERT_TRUE(v.has_value());
  }
});
producer.join();
consumer.join();
```

**No Snapshot Testing:**
- No snapshot tests; all assertions are explicit.

---

*Testing analysis: 2026-06-20*
*Update when test patterns change*
