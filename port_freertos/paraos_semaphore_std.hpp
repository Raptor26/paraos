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
