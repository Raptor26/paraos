# std::semaphore-style Semaphore API — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `paraos::counting_semaphore<LeastMaxValue>` and `paraos::binary_semaphore` (alias to `counting_semaphore<1>`), fully compatible with C++20 `std::counting_semaphore` / `std::binary_semaphore`, for PC, Unix, Windows and FreeRTOS ports.

**Architecture:** Follow the v1.3 `paraos::mutex` pattern: a thin PC wrapper over `std::counting_semaphore` in `port_pc/`, forwarding headers in `port_unix/` and `port_win/`, a native FreeRTOS implementation in `port_freertos/`, and a standalone CTest in `port_tests/`. Legacy `paraos::SemaphoreCounting` / `paraos::SemaphoreBinary` remain untouched.

**Tech Stack:** C++20, CMake ≥ 3.20, Clang/GCC, GoogleTest, clang-tidy, FreeRTOS POSIX simulator.

## Global Constraints

- Public API must be 1:1 with C++20 `std::counting_semaphore` / `std::binary_semaphore` (no default template argument).
- All files must compile with `-Wall -Wextra -Wpedantic -Werror`.
- `*_clang_tidy` presets must not produce new warnings.
- FreeRTOS runtime tests only validate construction/destruction on macOS POSIX simulator; scheduler cannot start reliably.
- Commits in Russian, Conventional Commits style, impersonal past participles.
- Update GSD planning documents (`.planning/PROJECT.md`, `REQUIREMENTS.md`, `ROADMAP.md`, `STATE.md`) for milestone v1.4.

---

## Milestone Setup (v1.4)

### Task 0: Initialize GSD milestone documents

**Files:**
- Modify: `.planning/PROJECT.md`
- Create/Modify: `.planning/REQUIREMENTS.md`
- Create/Modify: `.planning/ROADMAP.md`
- Modify: `.planning/STATE.md`

**Interfaces:**
- Consumes: `docs/superpowers/specs/2026-06-22-std-semaphore-design.md`
- Produces: Updated planning artifacts describing milestone v1.4 "std::semaphore-style Semaphore API".

- [ ] **Step 1: Update `.planning/PROJECT.md`**

Add a new "Current Milestone" section for v1.4 and move v1.3 to shipped history. Include:

```markdown
## Current Milestone: v1.4 std::semaphore-style Semaphore API

**Goal:** Добавить `paraos::counting_semaphore<LeastMaxValue>` и `paraos::binary_semaphore`, совместимые с C++20 `std::counting_semaphore` / `std::binary_semaphore`, для PC, Unix, Windows и FreeRTOS рядом с существующими `paraos::SemaphoreCounting` / `paraos::SemaphoreBinary`.

## Current State

**Shipped:** v1.3 std::mutex-style Mutex API (2026-06-22)

- ... (keep existing v1.3 bullets)

**Previously shipped:** v1.2 ... (keep existing)
```

- [ ] **Step 2: Create `.planning/REQUIREMENTS.md`**

Use template from `~/.kimi-code/gsd-core/templates/requirements.md`. Define requirements:

| ID | Requirement |
|----|-------------|
| SEM-01 | `paraos::counting_semaphore<LeastMaxValue>` provides `acquire()`, blocking until a unit is available. |
| SEM-02 | `paraos::counting_semaphore<LeastMaxValue>` provides `try_acquire()`, non-blocking, returning `bool`. |
| SEM-03 | `paraos::counting_semaphore<LeastMaxValue>` provides `release(update = 1)`. |
| SEM-04 | `paraos::counting_semaphore<LeastMaxValue>` provides `try_acquire_for(std::chrono::duration<Rep, Period>&)`. |
| SEM-05 | `paraos::counting_semaphore<LeastMaxValue>` provides `try_acquire_until(std::chrono::time_point<Clock, Duration>&)`. |
| SEM-06 | `paraos::counting_semaphore<LeastMaxValue>` provides `static constexpr max()`. |
| SEM-07 | `paraos::binary_semaphore` is a type alias to `paraos::counting_semaphore<1>`. |
| SEM-08 | Semaphore is non-copyable and non-movable. |
| SEM-09 | `port_pc/paraos_semaphore_std.hpp` implements PC/Unix/Windows semaphore over `std::counting_semaphore`. |
| SEM-10 | `port_unix/paraos_semaphore_std.hpp` forwards to `port_pc/paraos_semaphore_std.hpp`. |
| SEM-11 | `port_win/paraos_semaphore_std.hpp` forwards to `port_pc/paraos_semaphore_std.hpp`. |
| SEM-12 | `port_freertos/paraos_semaphore_std.hpp` implements semaphore over FreeRTOS counting semaphore API. |
| SEM-13 | `port_tests/test_semaphore_std.cpp` is created, registered in CTest, and passes on `pc_debug_clang` / `pc_debug_gcc`. |
| SEM-14 | `*_clang_tidy` presets build without new warnings. |

- [ ] **Step 3: Create `.planning/ROADMAP.md`**

Define phases (continuing numbering after v1.3 phase 15):

- **Phase 16: PC semaphore implementation** — implement `port_pc/paraos_semaphore_std.hpp` and forwarding headers (SEM-01..SEM-11).
- **Phase 17: FreeRTOS semaphore implementation** — implement `port_freertos/paraos_semaphore_std.hpp` (SEM-12).
- **Phase 18: Build, tests and static analysis** — add `test_semaphore_std.cpp`, run all presets (SEM-13, SEM-14).

- [ ] **Step 4: Update `.planning/STATE.md`**

Reset state for v1.4:

```yaml
gsd_state_version: 1.0
milestone: v1.4
milestone_name: "std::semaphore-style Semaphore API"
status: in_progress
last_updated: "2026-06-22"
last_activity: 2026-06-22
progress:
  total_phases: 3
  completed_phases: 0
  total_plans: 3
  completed_plans: 0
  percent: 0
```

- [ ] **Step 5: Commit milestone documents**

```bash
git add .planning/PROJECT.md .planning/REQUIREMENTS.md .planning/ROADMAP.md .planning/STATE.md
git commit -m "docs(planning): инициализирована веха v1.4 std::semaphore-style Semaphore API"
```

---

## Phase 16: PC Semaphore Implementation

### Task 1: Create `port_pc/paraos_semaphore_std.hpp`

**Files:**
- Create: `port_pc/paraos_semaphore_std.hpp`

**Interfaces:**
- Consumes: `std::counting_semaphore` from `<semaphore>`.
- Produces: `paraos::counting_semaphore<LeastMaxValue>`, `paraos::binary_semaphore`.

- [ ] **Step 1: Write the header**

```cpp
/// @file paraos_semaphore_std.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2025 Stilsoft
///
/// MIT License:
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the 'Software'), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.
///
/// @brief PC implementation of paraos::counting_semaphore as a thin wrapper
///        over std::counting_semaphore.

#ifndef PARAOS_SEMAPHORE_STD_HPP
#define PARAOS_SEMAPHORE_STD_HPP

#include <chrono>
#include <cstddef>
#include <semaphore>

namespace paraos {

/// @brief std::counting_semaphore-style wrapper for PC platforms.
///
/// Provides acquire(), try_acquire(), release(), try_acquire_for() and
/// try_acquire_until() with semantics matching std::counting_semaphore.
/// The type is non-copyable and non-movable. binary_semaphore is provided
/// as a type alias to counting_semaphore<1>.
template <std::ptrdiff_t LeastMaxValue>
class counting_semaphore {
 public:
  static_assert(LeastMaxValue > 0,
                "counting_semaphore: LeastMaxValue must be positive");

  /// @brief Construct the semaphore with the given initial counter value.
  explicit counting_semaphore(std::ptrdiff_t desired) : sem_(desired) {}

  /// @brief Destroy the semaphore.
  ~counting_semaphore() = default;

  /// @brief Copy construction is disabled.
  counting_semaphore(const counting_semaphore& other) = delete;

  /// @brief Copy assignment is disabled.
  auto operator=(const counting_semaphore& other)
      -> counting_semaphore& = delete;

  /// @brief Move construction is disabled.
  counting_semaphore(counting_semaphore&& other) noexcept = delete;

  /// @brief Move assignment is disabled.
  auto operator=(counting_semaphore&& other) noexcept
      -> counting_semaphore& = delete;

  /// @brief Maximum number of resources the semaphore can track.
  static constexpr auto max() noexcept -> std::ptrdiff_t {
    return std::counting_semaphore<LeastMaxValue>::max();
  }

  /// @brief Decrement the counter, blocking until a resource is available.
  void acquire() { sem_.acquire(); }

  /// @brief Increment the counter by @p update.
  void release(std::ptrdiff_t update = 1) { sem_.release(update); }

  /// @brief Try to decrement the counter without blocking.
  /// @return true if the counter was decremented, false otherwise.
  [[nodiscard]] auto try_acquire() noexcept -> bool {
    return sem_.try_acquire();
  }

  /// @brief Try to decrement the counter, blocking up to @p rel_time.
  template <class Rep, class Period>
  [[nodiscard]] auto try_acquire_for(
      const std::chrono::duration<Rep, Period>& rel_time) -> bool {
    return sem_.try_acquire_for(rel_time);
  }

  /// @brief Try to decrement the counter, blocking until @p abs_time.
  template <class Clock, class Duration>
  [[nodiscard]] auto try_acquire_until(
      const std::chrono::time_point<Clock, Duration>& abs_time) -> bool {
    return sem_.try_acquire_until(abs_time);
  }

 private:
  std::counting_semaphore<LeastMaxValue> sem_;
};

/// @brief std::binary_semaphore-style alias.
using binary_semaphore = counting_semaphore<1>;

}  // namespace paraos

#endif /* PARAOS_SEMAPHORE_STD_HPP */
```

- [ ] **Step 2: Create Unix forwarding header**

Create `port_unix/paraos_semaphore_std.hpp`:

```cpp
/// @file paraos_semaphore_std.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2025 Stilsoft
///
/// MIT License ...
///
/// @brief Unix forwarding header for paraos::counting_semaphore.

#ifndef PARAOS_UNIX_SEMAPHORE_STD_HPP
#define PARAOS_UNIX_SEMAPHORE_STD_HPP

#include "../port_pc/paraos_semaphore_std.hpp"

#endif /* PARAOS_UNIX_SEMAPHORE_STD_HPP */
```

- [ ] **Step 3: Create Windows forwarding header**

Create `port_win/paraos_semaphore_std.hpp`:

```cpp
/// @file paraos_semaphore_std.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2025 Stilsoft
///
/// MIT License ...
///
/// @brief Windows forwarding header for paraos::counting_semaphore.

#ifndef PARAOS_WIN_SEMAPHORE_STD_HPP
#define PARAOS_WIN_SEMAPHORE_STD_HPP

#include "../port_pc/paraos_semaphore_std.hpp"

#endif /* PARAOS_WIN_SEMAPHORE_STD_HPP */
```

- [ ] **Step 4: Build PC presets**

```bash
cmake --preset pc_debug_clang
cmake --build build/pc_debug_clang/ --target paraos -j
cmake --preset pc_debug_gcc
cmake --build build/pc_debug_gcc/ --target paraos -j
```

Expected: both builds succeed without warnings.

- [ ] **Step 5: Commit**

```bash
git add port_pc/paraos_semaphore_std.hpp port_unix/paraos_semaphore_std.hpp port_win/paraos_semaphore_std.hpp
git commit -m "feat(semaphore): добавлен paraos::counting_semaphore для PC, Unix и Windows"
```

---

## Phase 17: FreeRTOS Semaphore Implementation

### Task 2: Create `port_freertos/paraos_semaphore_std.hpp`

**Files:**
- Create: `port_freertos/paraos_semaphore_std.hpp`

**Interfaces:**
- Consumes: `FreeRTOS.h`, `semphr.h`, `etl/error_handler.h`, `paraos_utils.hpp` (`PARAOS_ConvertMsToTicks`, `delay_type`, `max_delay`).
- Produces: `paraos::counting_semaphore<LeastMaxValue>`, `paraos::binary_semaphore`.

- [ ] **Step 1: Write the header**

```cpp
/// @file paraos_semaphore_std.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2025 Stilsoft
///
/// MIT License ...
///
/// @brief FreeRTOS implementation of paraos::counting_semaphore over the
///        FreeRTOS counting semaphore API.

#ifndef PARAOS_FREERTOS_SEMAPHORE_STD_HPP
#define PARAOS_FREERTOS_SEMAPHORE_STD_HPP

#include <chrono>
#include <cstddef>
#include <new>
#include <ratio>
#include <stdexcept>

#include "FreeRTOS.h"
#include "etl/error_handler.h"
#include "paraos_utils.hpp"
#include "semphr.h"

namespace paraos {

/// @brief std::counting_semaphore-style wrapper for FreeRTOS.
///
/// Provides acquire(), try_acquire(), release(), try_acquire_for() and
/// try_acquire_until() with semantics matching std::counting_semaphore.
/// The type is non-copyable and non-movable. binary_semaphore is provided
/// as a type alias to counting_semaphore<1>.
template <std::ptrdiff_t LeastMaxValue>
class counting_semaphore {
 public:
  static_assert(LeastMaxValue > 0,
                "counting_semaphore: LeastMaxValue must be positive");

  /// @brief Construct the semaphore with the given initial counter value.
  explicit counting_semaphore(std::ptrdiff_t desired)
      : handle_(xSemaphoreCreateCounting(
            static_cast<UBaseType_t>(max()),
            static_cast<UBaseType_t>(desired))) {
    ETL_ASSERT(handle_ != nullptr, std::bad_alloc());
  }

  /// @brief Destroy the semaphore and delete the underlying handle.
  ~counting_semaphore() {
    if (handle_ != nullptr) {
      vSemaphoreDelete(handle_);
    }
  }

  /// @brief Copy construction is disabled.
  counting_semaphore(const counting_semaphore& other) = delete;

  /// @brief Copy assignment is disabled.
  auto operator=(const counting_semaphore& other)
      -> counting_semaphore& = delete;

  /// @brief Move construction is disabled.
  counting_semaphore(counting_semaphore&& other) noexcept = delete;

  /// @brief Move assignment is disabled.
  auto operator=(counting_semaphore&& other) noexcept
      -> counting_semaphore& = delete;

  /// @brief Maximum number of resources the semaphore can track.
  static constexpr auto max() noexcept -> std::ptrdiff_t {
    return LeastMaxValue;
  }

  /// @brief Decrement the counter, blocking until a resource is available.
  void acquire() { (void)xSemaphoreTake(handle_, portMAX_DELAY); }

  /// @brief Increment the counter by @p update.
  void release(std::ptrdiff_t update = 1) {
    ETL_ASSERT(handle_ != nullptr, std::runtime_error("semaphore not valid"));

    // Suspend the scheduler to perform a multi-step release atomically and
    // to detect overflow before any Give reaches the kernel.
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while)
    do {
      if (GetCurrentCount() + update > max()) {
        ETL_ASSERT_FAIL(std::runtime_error("semaphore release overflow"));
      }
    } while (false);

    vTaskSuspendAll();
    for (std::ptrdiff_t i = 0; i < update; ++i) {
      (void)xSemaphoreGive(handle_);
    }
    (void)xTaskResumeAll();
  }

  /// @brief Try to decrement the counter without blocking.
  /// @return true if the counter was decremented, false otherwise.
  [[nodiscard]] auto try_acquire() noexcept -> bool {
    return xSemaphoreTake(handle_, 0) == pdTRUE;
  }

  /// @brief Try to decrement the counter, blocking up to @p rel_time.
  template <class Rep, class Period>
  [[nodiscard]] auto try_acquire_for(
      const std::chrono::duration<Rep, Period>& rel_time) -> bool {
    const auto ms = std::chrono::ceil<std::chrono::milliseconds>(rel_time);
    return xSemaphoreTake(handle_, PARAOS_ConvertMsToTicks(
                                        static_cast<delay_type>(ms.count())))
           == pdTRUE;
  }

  /// @brief Try to decrement the counter, blocking until @p abs_time.
  template <class Clock, class Duration>
  [[nodiscard]] auto try_acquire_until(
      const std::chrono::time_point<Clock, Duration>& abs_time) -> bool {
    const auto now = Clock::now();
    if (abs_time <= now) {
      return try_acquire();
    }
    return try_acquire_for(abs_time - now);
  }

 private:
  [[nodiscard]] auto GetCurrentCount() const noexcept -> std::ptrdiff_t {
    return static_cast<std::ptrdiff_t>(uxSemaphoreGetCount(handle_));
  }

  SemaphoreHandle_t handle_;
};

/// @brief std::binary_semaphore-style alias.
using binary_semaphore = counting_semaphore<1>;

}  // namespace paraos

#endif /* PARAOS_FREERTOS_SEMAPHORE_STD_HPP */
```

- [ ] **Step 2: Build FreeRTOS presets**

```bash
cmake --preset freertos_debug_clang
cmake --build build/freertos_debug_clang/ --target paraos -j
cmake --preset freertos_debug_gcc
cmake --build build/freertos_debug_gcc/ --target paraos -j
```

Expected: both builds succeed without warnings.

- [ ] **Step 3: Commit**

```bash
git add port_freertos/paraos_semaphore_std.hpp
git commit -m "feat(semaphore): добавлен paraos::counting_semaphore для FreeRTOS"
```

---

## Phase 18: Build, Tests and Static Analysis

### Task 3: Add `port_tests/test_semaphore_std.cpp`

**Files:**
- Create: `port_tests/test_semaphore_std.cpp`
- Modify: `port_tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `paraos_semaphore_std.hpp`.
- Produces: CTest-registered `test_semaphore_std` executable.

- [ ] **Step 1: Write the test**

```cpp
/// @file test_semaphore_std.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2025 Stilsoft
///
/// MIT License ...
///
/// @brief Basic standalone test for paraos::counting_semaphore and
///        paraos::binary_semaphore.

#include <cstdlib>

#include "paraos_semaphore_std.hpp"

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables,
// *-readability-identifier-naming)

#if !defined(PARAOS_LIKE_FREERTOS)
#include <atomic>
#include <chrono>
#include <iostream>

namespace {
std::atomic<bool> is_test_passed{false};

auto RunTests() -> bool {
  paraos::counting_semaphore<2> sem_counting(0);
  paraos::binary_semaphore sem_binary(0);

  // counting_semaphore basic acquire/release
  sem_counting.release();
  sem_counting.acquire();
  if (!sem_counting.try_acquire()) {
    return false;
  }

  sem_counting.release(2);
  if (!sem_counting.try_acquire()) {
    return false;
  }
  sem_counting.acquire();

  // binary_semaphore basic
  sem_binary.release();
  sem_binary.acquire();
  if (sem_binary.try_acquire()) {
    return false;
  }

  // try_acquire_for timeout
  paraos::binary_semaphore sem_empty(0);
  const auto start = std::chrono::steady_clock::now();
  const bool timed_out =
      !sem_empty.try_acquire_for(std::chrono::milliseconds(50));
  const auto elapsed = std::chrono::steady_clock::now() - start;
  if (!timed_out) {
    return false;
  }
  if (elapsed < std::chrono::milliseconds(50)) {
    return false;
  }

  // try_acquire_for success
  paraos::binary_semaphore sem_full(1);
  if (!sem_full.try_acquire_for(std::chrono::milliseconds(50))) {
    return false;
  }

  // max()
  if (paraos::counting_semaphore<2>::max() < 2) {
    return false;
  }
  if (paraos::binary_semaphore::max() != 1) {
    return false;
  }

  return true;
}
}  // namespace
#endif

auto main() -> int {
#if defined(PARAOS_LIKE_FREERTOS)
  {
    paraos::counting_semaphore<2> sem_counting(0);
    paraos::binary_semaphore sem_binary(0);
    (void)sem_counting;
    (void)sem_binary;
  }
  return EXIT_SUCCESS;
#else
  if (RunTests()) {
    is_test_passed.store(true);
    std::cout << "OK\n";
    return EXIT_SUCCESS;
  }

  std::cout << "FAIL\n";
  return EXIT_FAILURE;
#endif
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables,
// *-readability-identifier-naming)
```

- [ ] **Step 2: Modify `port_tests/CMakeLists.txt`**

Insert after the `test_mutex_basic` block:

```cmake
# semaphore std ----------------------------------------------------------------
add_executable(test_semaphore_std test_semaphore_std.cpp)
target_link_libraries(test_semaphore_std PRIVATE paraos::paraos)

target_compile_features(test_semaphore_std PRIVATE cxx_std_20 c_std_11)
target_compile_options(test_semaphore_std PRIVATE -Wall -Wextra -Wpedantic
                                                  -Werror)

add_test(NAME test_semaphore_std COMMAND test_semaphore_std)

if(CLANG_TIDY_ENABLE)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}")
    set_target_properties(test_semaphore_std PROPERTIES CXX_CLANG_TIDY
                                                        "${DO_CLANG_TIDY}")
  endif()
endif()
```

- [ ] **Step 3: Build and run PC tests**

```bash
cmake --build build/pc_debug_clang/ --target test_semaphore_std -j
ctest --test-dir build/pc_debug_clang/ -R test_semaphore_std --output-on-failure

cmake --build build/pc_debug_gcc/ --target test_semaphore_std -j
ctest --test-dir build/pc_debug_gcc/ -R test_semaphore_std --output-on-failure
```

Expected: `test_semaphore_std` passes.

- [ ] **Step 4: Build FreeRTOS tests**

```bash
cmake --build build/freertos_debug_clang/ --target test_semaphore_std -j
ctest --test-dir build/freertos_debug_clang/ -R test_semaphore_std --output-on-failure

cmake --build build/freertos_debug_gcc/ --target test_semaphore_std -j
ctest --test-dir build/freertos_debug_gcc/ -R test_semaphore_std --output-on-failure
```

Expected: compiles; on macOS POSIX simulator only construction/destruction path runs (returns EXIT_SUCCESS).

- [ ] **Step 5: Run clang-tidy presets**

```bash
cmake --preset pc_debug_gcc_clang_tidy
cmake --build build/pc_debug_gcc_clang_tidy/ --target test_semaphore_std -j
```

If available on this machine, also run `freertos_debug_gcc_clang_tidy`.

Expected: no new warnings originating from `paraos_semaphore_std.hpp` or `test_semaphore_std.cpp`.

- [ ] **Step 6: Full regression run**

```bash
ctest --test-dir build/pc_debug_clang/ --output-on-failure --stop-on-failure --schedule-random --timeout 20
ctest --test-dir build/pc_debug_gcc/ --output-on-failure --stop-on-failure --schedule-random --timeout 20
```

Expected: all tests pass (54/54 or current count).

- [ ] **Step 7: Commit**

```bash
git add port_tests/test_semaphore_std.cpp port_tests/CMakeLists.txt
git commit -m "test(semaphore): добавлен базовый тест paraos::counting_semaphore / binary_semaphore"
```

---

## Final Verification & Milestone Close

### Task 4: Final verification and milestone documentation update

**Files:**
- Modify: `.planning/PROJECT.md`
- Modify: `.planning/REQUIREMENTS.md`
- Modify: `.planning/ROADMAP.md`
- Modify: `.planning/STATE.md`

- [ ] **Step 1: Mark requirements as verified**

Update `.planning/REQUIREMENTS.md` with `[x]` for SEM-01..SEM-14 and add traceability to phases.

- [ ] **Step 2: Mark phases complete in ROADMAP**

Update `.planning/ROADMAP.md`: Phases 16, 17, 18 completed.

- [ ] **Step 3: Update PROJECT.md**

Move v1.4 to "Shipped" with summary bullets:

```markdown
**Shipped:** v1.4 std::semaphore-style Semaphore API (2026-06-22)

- Добавлены `paraos::counting_semaphore<LeastMaxValue>` и `paraos::binary_semaphore`.
- PC, Unix и Windows используют `std::counting_semaphore` через `port_pc/paraos_semaphore_std.hpp`.
- FreeRTOS реализован поверх `xSemaphoreCreateCounting` / `xSemaphoreTake` / `xSemaphoreGive`.
- Обеспечена совместимость с `std::chrono` таймаутами (`try_acquire_for`, `try_acquire_until`).
- Добавлен тест `port_tests/test_semaphore_std.cpp` и зарегистрирован в CTest.
- PC-пресеты собираются и проходят `ctest`; `*_clang_tidy` пресеты без новых предупреждений.
```

- [ ] **Step 4: Update STATE.md**

Set `status: complete`, `completed_phases: 3`, `completed_plans: 3`, `percent: 100`.

- [ ] **Step 5: Commit milestone close**

```bash
git add .planning/PROJECT.md .planning/REQUIREMENTS.md .planning/ROADMAP.md .planning/STATE.md
git commit -m "docs(planning): завершена веха v1.4 std::semaphore-style Semaphore API"
```

---

## Self-Review Checklist

- [ ] Spec coverage: every SEM requirement maps to a task.
- [ ] No placeholders ("TBD", "TODO", "fill in", etc.).
- [ ] Type consistency: `counting_semaphore<LeastMaxValue>` used everywhere.
- [ ] File paths are exact and relative to repo root.
- [ ] Commit messages follow Conventional Commits in Russian, impersonal past participles.

---
*Plan created: 2026-06-22*
