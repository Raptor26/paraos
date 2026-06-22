# std::semaphore-style Semaphore API — Design Spec

## Goal

Add a `std::counting_semaphore` / `std::binary_semaphore`-compatible API to PARAOS, named `paraos::counting_semaphore` and `paraos::binary_semaphore`, next to the legacy `paraos::SemaphoreCounting` / `paraos::SemaphoreBinary`.

## Motivation

v1.3 added `paraos::mutex` as a `std::mutex`-compatible primitive. The next logical step is a `std::semaphore`-compatible primitive so that application code can use standard C++20 synchronization interfaces across PC and FreeRTOS builds without platform `#ifdef`s.

## Public API

```cpp
namespace paraos {

template<std::ptrdiff_t LeastMaxValue>
class counting_semaphore {
 public:
  explicit counting_semaphore(std::ptrdiff_t desired);
  ~counting_semaphore();

  counting_semaphore(const counting_semaphore&) = delete;
  counting_semaphore& operator=(const counting_semaphore&) = delete;
  counting_semaphore(counting_semaphore&&) = delete;
  counting_semaphore& operator=(counting_semaphore&&) = delete;

  static constexpr std::ptrdiff_t max() noexcept;

  void acquire();
  void release(std::ptrdiff_t update = 1);
  [[nodiscard]] bool try_acquire() noexcept;

  template<class Rep, class Period>
  [[nodiscard]] bool try_acquire_for(
      const std::chrono::duration<Rep, Period>& rel_time);

  template<class Clock, class Duration>
  [[nodiscard]] bool try_acquire_until(
      const std::chrono::time_point<Clock, Duration>& abs_time);
};

using binary_semaphore = counting_semaphore<1>;

}  // namespace paraos
```

This is intentionally 1:1 with C++20 `<semaphore>`:

| `std` | `paraos` |
|-------|----------|
| `std::counting_semaphore<LeastMaxValue>` | `paraos::counting_semaphore<LeastMaxValue>` |
| `std::binary_semaphore` | `paraos::binary_semaphore` (alias to `counting_semaphore<1>`) |

## Decisions

| Decision | Rationale |
|----------|-----------|
| `release(update)` overflow: `ETL_ASSERT(..., std::runtime_error(...))` | Matches `std::counting_semaphore` behavior and the existing `paraos::mutex` FreeRTOS implementation style. |
| FreeRTOS timeouts use `std::chrono` with millisecond rounding up | FreeRTOS ticks are millisecond-granular on the supported PC simulator; rounding up avoids premature timeouts. |
| `acquire()` | `acquire()` |
| `release(update = 1)` | `release(update = 1)` |
| `try_acquire()` | `try_acquire()` |
| `try_acquire_for(...)` | `try_acquire_for(...)` |
| `try_acquire_until(...)` | `try_acquire_until(...)` |
| `max()` | `max()` |

## File Layout

Following the v1.3 `mutex` pattern:

| File | Responsibility |
|------|----------------|
| `port_pc/paraos_semaphore_std.hpp` | PC implementation as a thin wrapper over `std::counting_semaphore`. |
| `port_unix/paraos_semaphore_std.hpp` | Forwarding header to `port_pc/paraos_semaphore_std.hpp`. |
| `port_win/paraos_semaphore_std.hpp` | Forwarding header to `port_pc/paraos_semaphore_std.hpp`. |
| `port_freertos/paraos_semaphore_std.hpp` | FreeRTOS implementation over `xSemaphoreCreateCounting` / `xSemaphoreTake` / `xSemaphoreGive`. |
| `port_tests/test_semaphore_std.cpp` | Standalone test exercising `paraos::counting_semaphore` and `paraos::binary_semaphore`. |

The legacy `paraos_semaphore.hpp` files in each port remain untouched.

## PC Implementation (`port_pc/paraos_semaphore_std.hpp`)

```cpp
template<std::ptrdiff_t LeastMaxValue>
class counting_semaphore {
 public:
  static_assert(LeastMaxValue > 0,
                "counting_semaphore: LeastMaxValue must be positive");

  explicit counting_semaphore(std::ptrdiff_t desired) : sem_(desired) {}
  ~counting_semaphore() = default;

  counting_semaphore(const counting_semaphore&) = delete;
  auto operator=(const counting_semaphore&) -> counting_semaphore& = delete;
  counting_semaphore(counting_semaphore&&) = delete;
  auto operator=(counting_semaphore&&) -> counting_semaphore& = delete;

  static constexpr auto max() noexcept -> std::ptrdiff_t {
    return std::counting_semaphore<LeastMaxValue>::max();
  }

  void acquire() { sem_.acquire(); }

  void release(std::ptrdiff_t update = 1) { sem_.release(update); }

  [[nodiscard]] auto try_acquire() noexcept -> bool {
    return sem_.try_acquire();
  }

  template<class Rep, class Period>
  [[nodiscard]] auto try_acquire_for(
      const std::chrono::duration<Rep, Period>& rel_time) -> bool {
    return sem_.try_acquire_for(rel_time);
  }

  template<class Clock, class Duration>
  [[nodiscard]] auto try_acquire_until(
      const std::chrono::time_point<Clock, Duration>& abs_time) -> bool {
    return sem_.try_acquire_until(abs_time);
  }

 private:
  std::counting_semaphore<LeastMaxValue> sem_;
};
```

## FreeRTOS Implementation (`port_freertos/paraos_semaphore_std.hpp`)

- `max()` returns `LeastMaxValue`.
- Constructor creates `xSemaphoreCreateCounting(max(), desired)` and asserts non-null.
- `acquire()` → `xSemaphoreTake(handle_, portMAX_DELAY)`.
- `try_acquire()` → `xSemaphoreTake(handle_, 0)`.
- `try_acquire_for(rel_time)` → accept any `std::chrono::duration<Rep, Period>`, convert to milliseconds via `std::chrono::duration_cast<std::chrono::milliseconds>` with `std::chrono::ceil` semantics (round up to the next whole millisecond), then pass to `xSemaphoreTake(handle_, PARAOS_ConvertMsToTicks(ms))`.
- `try_acquire_until(abs_time)` → accept any `std::chrono::time_point<Clock, Duration>`, compute remaining duration as `abs_time - Clock::now()`, clamp negative durations to zero, and delegate to `try_acquire_for`.
- `release(update)` → loop `xSemaphoreGive` `update` times. If `update` would exceed `max()`, report failure via `ETL_ASSERT(handle_ != nullptr, std::runtime_error(...))` matching the `std::counting_semaphore` overflow semantics and the existing `paraos::mutex` FreeRTOS implementation style. The loop runs inside a scheduler suspension (`vTaskSuspendAll` / `xTaskResumeAll`) to keep the multi-step release atomic.
- Destructor deletes the semaphore if non-null.
- Non-copyable, non-movable.

## Tests (`port_tests/test_semaphore_std.cpp`)

A standalone executable registered in CTest, similar to `test_mutex_basic.cpp`:

1. Construct / destroy `counting_semaphore<2>`.
2. Construct / destroy `binary_semaphore`.
3. `release()` then `acquire()`.
4. `try_acquire()` returns true after release, false otherwise.
5. `try_acquire_for(std::chrono::milliseconds(50))` times out on empty semaphore.
6. `try_acquire_for(std::chrono::milliseconds(50))` succeeds after release.
7. `release(2)` / `acquire()` twice on `counting_semaphore<2>`.
8. On FreeRTOS POSIX simulator (macOS), only construction/destruction is validated at runtime; the scheduler cannot start reliably in this environment.

## Build Integration

- Add `test_semaphore_std` target in `port_tests/CMakeLists.txt`.
- Compile with C++20 (`cxx_std_20`), `-Wall -Wextra -Wpedantic -Werror`.
- Attach `CXX_CLANG_TIDY` when `CLANG_TIDY_ENABLE` is set.
- Register with `add_test(NAME test_semaphore_std COMMAND test_semaphore_std)`.

## Static Analysis

- `*_clang_tidy` presets must not produce new warnings from the new headers or test.
- Use `NOLINTBEGIN/NOLINTEND` with rationale comments if a specific warning is unavoidable, consistent with project style.

## Out of Scope

- Replacing legacy `paraos::SemaphoreCounting` / `paraos::SemaphoreBinary`.
- `std::latch` or `std::barrier`.
- Runtime verification on physical FreeRTOS targets or Windows host.
- Changes to `extra/` or `containers/` consumers.

## Traceability

| Requirement | v1.4 |
|-------------|------|
| `counting_semaphore<LeastMaxValue>` public API 1:1 with `std::counting_semaphore` | v1.4 |
| `binary_semaphore` as alias to `counting_semaphore<1>` | v1.4 |
| PC implementation via `std::counting_semaphore` | v1.4 |
| FreeRTOS implementation via FreeRTOS counting semaphore API | v1.4 |
| Standalone CTest test | v1.4 |
| No new clang-tidy warnings | v1.4 |

---
*Defined: 2026-06-22*
