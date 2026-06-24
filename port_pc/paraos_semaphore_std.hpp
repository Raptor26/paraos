/// @file paraos_semaphore_std.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
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
  void release(std::ptrdiff_t update = 1) {
    if (update < 0) {
      throw std::runtime_error(
          "semaphore release update must be non-negative");
    }
    if (update > max() - GetCurrentCount()) {
      throw std::runtime_error("semaphore release overflow");
    }
    sem_.release(update);
  }

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
  [[nodiscard]] auto GetCurrentCount() -> std::ptrdiff_t {
    // std::counting_semaphore does not expose its current count.  This helper
    // takes a best-effort snapshot by draining and re-filling the semaphore.
    // It is inherently racy under contention and is used only for validation.
    std::ptrdiff_t count{0};
    while (sem_.try_acquire()) {
      ++count;
    }
    if (count > 0) {
      sem_.release(count);
    }
    return count;
  }

  std::counting_semaphore<LeastMaxValue> sem_;
};

/// @brief std::binary_semaphore-style alias.
using binary_semaphore = counting_semaphore<1>;

}  // namespace paraos

#endif /* PARAOS_SEMAPHORE_STD_HPP */
