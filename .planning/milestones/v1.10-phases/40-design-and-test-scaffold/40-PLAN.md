---
wave: 40
depends_on: []
files_modified:
  - port_unix/paraos_timer.hpp
  - port_tests/test_timer.cpp
  - port_tests/CMakeLists.txt
autonomous: true
requirements:
  - TMR-04
  - TEST-01
---

# Phase 40: Design and test scaffold

**Goal:** Подготовить скелет новой реализации и тестовую инфраструктуру до начала разработки цикла таймера.

**Milestone:** PARAOS v1.10 — Modernize Unix timer with paraos primitives.

**Requirements covered:** TMR-04, TEST-01

**Phase boundary:** This phase is infrastructure-only. No functional timer loop behavior is implemented; only the header skeleton, a minimal standalone test file, and CMake wiring are delivered.

---

## Tasks

<tasks>

<task>
  <id>40.1</id>
  <title>Refactor `port_unix/paraos_timer.hpp` to a jthread/mutex/semaphore skeleton</title>
  <requirements>TMR-04, TEST-01</requirements>
  <read_first>
    - port_unix/paraos_timer.hpp
    - port_unix/paraos_jthread.hpp
    - port_unix/paraos_mutex_std.hpp
    - port_unix/paraos_semaphore_std.hpp
    - port_unix/paraos_sleep.hpp
    - port_tests/CMakeLists.txt
  </read_first>
  <action>
    Rewrite `port_unix/paraos_timer.hpp` so that:
    1. Includes added: `#include "paraos_jthread.hpp"`, `#include "paraos_mutex_std.hpp"`, `#include "paraos_semaphore_std.hpp"`, `#include "paraos_sleep.hpp"`, `#include <optional>`.
    2. POSIX/pthread includes removed: `<pthread.h>`, `<signal.h>`, `<time.h>`, `<unistd.h>`, `<stdio.h>`, `<stdlib.h>`, `<cstring>`. Replace `<stdint.h>` with `<cstddef>` (for `std::size_t`).
    3. Public API signatures preserved exactly:
       - `explicit timer(std::size_t period_ms, bool start_immediately = false, bool is_auto_reload = true, std::string_view name = "Timer")`
       - `auto start(paraos::delay_type max_block_time = max_delay, bool is_isr = false) -> isr_bool`
       - `auto stop(paraos::delay_type max_block_time = max_delay, bool is_isr = false) -> isr_bool`
       - `auto reset(paraos::delay_type max_block_time = max_delay, bool is_isr = false) -> isr_bool`
       - `auto change_period(std::size_t period_ms, paraos::delay_type max_block_time = max_delay, bool is_isr = false) -> isr_bool`
       - `virtual void run()`
       - Deleted copy/move operations and deprecated aliases `Start`, `ChangePeriod`, `Stop`, `Reset`, `Timer`.
    4. Constructor body initializes fields directly and, if `start_immediately` is true, calls `start()`. Remove the `create()` helper and any `timer_create` / `pthread_mutex_init` / `pthread_cond_init` calls.
    5. Destructor body calls `stop()` and has no `timer_delete` / `pthread_mutex_destroy` / `pthread_cond_destroy`.
    6. `start()`, `stop()`, `reset()`, `change_period()` bodies replaced with placeholder implementations that return `isr_bool{true}` (or equivalent) and do not call POSIX/pthread APIs.
    7. Private member section cleaned:
       - Remove `timer_t timer_id_`, `pthread_t thread_`, `pthread_mutex_t mutex_`, `pthread_cond_t cond_`.
       - Remove all `#ifdef __linux__` / `#elif defined(__APPLE__)` / `#else #error` blocks inside the class.
       - Keep `std::size_t period_ms_`, `bool is_auto_reload_`, `std::string_view name_`.
       - Add `paraos::mutex mutex_`, `paraos::binary_semaphore wake_sem_{0}`, `std::optional<paraos::jthread> worker_`, `bool is_running_{false}`, `bool is_stop_requested_{false}`.
  </action>
  <acceptance_criteria>
    - `grep -E 'timer_create|timer_settime|timer_delete|pthread_mutex_|pthread_cond_|pthread_create|pthread_join' port_unix/paraos_timer.hpp` returns no matches.
    - `grep -E '#ifdef __linux__|#elif defined\(__APPLE__\)|#else.*Unsupported' port_unix/paraos_timer.hpp` returns no matches.
    - `grep -E 'paraos_jthread\.hpp|paraos_mutex_std\.hpp|paraos_semaphore_std\.hpp|paraos_sleep\.hpp' port_unix/paraos_timer.hpp` returns matches.
    - Public constructor signature is exactly `explicit timer(std::size_t period_ms, bool start_immediately = false, bool is_auto_reload = true, std::string_view name = "Timer")`.
    - Public method signatures for `start`, `stop`, `reset`, `change_period`, `run` match the existing signatures from the baseline header.
    - File compiles as part of `cmake --build build/pc_debug_clang` and `cmake --build build/pc_debug_gcc` without errors.
  </acceptance_criteria>
</task>

<task>
  <id>40.2</id>
  <title>Create minimal standalone `port_tests/test_timer.cpp` skeleton</title>
  <requirements>TMR-04, TEST-01</requirements>
  <read_first>
    - port_unix/paraos_timer.hpp
    - port_tests/test_jthread_basic.cpp
    - port_tests/test_semaphore_std.cpp
    - AGENTS.md
  </read_first>
  <action>
    Create `port_tests/test_timer.cpp` containing:
    1. Doxygen `@file` comment with `@brief` describing the standalone timer skeleton test.
    2. `#include "paraos_timer.hpp"`.
    3. A class `test_timer_app` (or equivalent) publicly derived from `paraos::timer`.
    4. Constructor forwards to `paraos::timer(100, false, true, "test_timer")`.
    5. Empty `void run() override {}`.
    6. `auto main() -> int` that instantiates `test_timer_app`, calls `start()`, calls `stop()`, and returns 0.
    7. No use of `pthread_*`, `timer_*`, or POSIX headers.
    8. Follow project snake_case naming and RAII conventions.
  </action>
  <acceptance_criteria>
    - File exists at `port_tests/test_timer.cpp`.
    - `grep -E 'class .* : .* paraos::timer' port_tests/test_timer.cpp` returns a match.
    - `grep -E 'void run\(\) override' port_tests/test_timer.cpp` returns a match.
    - `grep -E 'int main\(\)' port_tests/test_timer.cpp` returns a match.
    - File compiles with `pc_debug_clang` and `pc_debug_gcc` presets without errors.
    - Running the produced `test_timer` executable exits with code 0.
  </acceptance_criteria>
</task>

<task>
  <id>40.3</id>
  <title>Wire `test_timer` target in `port_tests/CMakeLists.txt`</title>
  <requirements>TMR-04, TEST-01</requirements>
  <read_first>
    - port_tests/CMakeLists.txt
    - AGENTS.md
  </read_first>
  <action>
    In `port_tests/CMakeLists.txt`, add after the `test_semaphore_std` block and before the GTest section:
    1. `add_executable(test_timer test_timer.cpp)`.
    2. `target_link_libraries(test_timer PRIVATE paraos::paraos)`.
    3. `target_compile_features(test_timer PRIVATE cxx_std_20 c_std_11)`.
    4. `target_compile_options(test_timer PRIVATE -Wall -Wextra -Wpedantic -Werror)`.
    5. `add_test(NAME test_timer COMMAND test_timer TIMEOUT 20)`.
    6. Inside the existing `if(CLANG_TIDY_ENABLE)` block, append `set_target_properties(test_timer PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")`.
  </action>
  <acceptance_criteria>
    - `grep -E 'add_executable\(test_timer' port_tests/CMakeLists.txt` returns a match.
    - `grep -E 'add_test\(NAME test_timer' port_tests/CMakeLists.txt` returns a match.
    - `grep -E 'TIMEOUT 20' port_tests/CMakeLists.txt` shows `test_timer` registration with timeout 20.
    - `grep -E 'set_target_properties\(test_timer PROPERTIES CXX_CLANG_TIDY' port_tests/CMakeLists.txt` returns a match.
    - After configuring `pc_debug_clang` and `pc_debug_gcc`, `ctest -N --test-dir build/pc_debug_clang` lists `test_timer`.
    - `cmake --build build/pc_debug_clang --target test_timer` succeeds.
    - `cmake --build build/pc_debug_gcc --target test_timer` succeeds.
  </acceptance_criteria>
</task>

</tasks>

---

## must_haves

1. The public API signature of `paraos::timer` is unchanged: constructor `(std::size_t period_ms, bool start_immediately = false, bool is_auto_reload = true, std::string_view name = "Timer")`; methods `start()`, `stop()`, `reset()`, `change_period()`; virtual `run()`.
2. `port_unix/paraos_timer.hpp` no longer contains `timer_create`, `timer_settime`, `timer_delete`, `pthread_mutex_*`, `pthread_cond_*`, `pthread_create`, or `pthread_join`.
3. `port_unix/paraos_timer.hpp` includes `paraos_jthread.hpp`, `paraos_mutex_std.hpp`, `paraos_semaphore_std.hpp`, and `paraos_sleep.hpp`.
4. `port_tests/test_timer.cpp` exists and contains a `paraos::timer` subclass with an empty `run()` override and a `main()` function.
5. `port_tests/CMakeLists.txt` registers a standalone `test_timer` target with `cxx_std_20`, `TIMEOUT 20`, and `CXX_CLANG_TIDY` attachment.
6. `test_timer` is visible in `ctest -N` for PC presets and compiles in both `pc_debug_clang` and `pc_debug_gcc`.

---

## Verification

1. Configure and build PC presets:
   ```bash
   cmake --preset pc_debug_clang
   cmake --build build/pc_debug_clang --target test_timer
   cmake --preset pc_debug_gcc
   cmake --build build/pc_debug_gcc --target test_timer
   ```
2. Confirm CTest registration:
   ```bash
   ctest -N --test-dir build/pc_debug_clang | grep -E 'test_timer'
   ctest -N --test-dir build/pc_debug_gcc | grep -E 'test_timer'
   ```
3. Run the new test:
   ```bash
   ctest --test-dir build/pc_debug_clang -R test_timer --output-on-failure
   ctest --test-dir build/pc_debug_gcc -R test_timer --output-on-failure
   ```
4. Confirm no POSIX/pthread timer API remains:
   ```bash
   grep -E 'timer_create|timer_settime|timer_delete|pthread_mutex_|pthread_cond_|pthread_create|pthread_join' port_unix/paraos_timer.hpp
   ```
   Expected: empty output.
5. If a `*_clang_tidy` preset is available (e.g., `pc_debug_gcc_clang_tidy`), build it and confirm `port_unix/paraos_timer.hpp` and `port_tests/test_timer.cpp` produce no new warnings.

---

## Artifacts this phase produces

### Files created
- `port_tests/test_timer.cpp`

### Files modified
- `port_unix/paraos_timer.hpp`
- `port_tests/CMakeLists.txt`

### Symbols preserved / introduced
- `paraos::timer` class
  - `explicit timer(std::size_t period_ms, bool start_immediately = false, bool is_auto_reload = true, std::string_view name = "Timer")`
  - `auto start(paraos::delay_type max_block_time = max_delay, bool is_isr = false) -> isr_bool`
  - `auto stop(paraos::delay_type max_block_time = max_delay, bool is_isr = false) -> isr_bool`
  - `auto reset(paraos::delay_type max_block_time = max_delay, bool is_isr = false) -> isr_bool`
  - `auto change_period(std::size_t period_ms, paraos::delay_type max_block_time = max_delay, bool is_isr = false) -> isr_bool`
  - `virtual void run()`
  - Deleted copy/move constructors and assignment operators
  - Deprecated aliases: `Start`, `ChangePeriod`, `Stop`, `Reset`, `Timer`
- New private data members (placeholders) in `paraos::timer`:
  - `paraos::mutex mutex_`
  - `paraos::binary_semaphore wake_sem_{0}`
  - `std::optional<paraos::jthread> worker_`
  - `bool is_running_{false}`
  - `bool is_stop_requested_{false}`
- CMake target / CTest test
  - `test_timer` executable target
  - `test_timer` CTest entry with `TIMEOUT 20`

### Build / CLI surface
- `cmake --build build/pc_debug_clang --target test_timer`
- `cmake --build build/pc_debug_gcc --target test_timer`
- `ctest --test-dir build/pc_debug_clang -R test_timer`
- `ctest --test-dir build/pc_debug_gcc -R test_timer`
