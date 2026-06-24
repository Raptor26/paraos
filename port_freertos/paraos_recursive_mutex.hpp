/// @file paraos_recursive_mutex.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_FREERTOS_RECURSIVE_MUTEX_HPP
#define PARAOS_FREERTOS_RECURSIVE_MUTEX_HPP

#include <new>

#include "FreeRTOS.h"
#include "etl/error_handler.h"
#include "semphr.h"

namespace paraos {

/// @brief std::recursive_mutex-style wrapper for FreeRTOS.
///
/// Provides lock(), try_lock(), and unlock() with semantics matching
/// std::recursive_mutex. The type is non-copyable and non-movable, and is
/// compatible with std::lock_guard and std::unique_lock.
class recursive_mutex {
 public:
  /// @brief Construct the recursive mutex by creating a FreeRTOS recursive
  /// mutex semaphore.
  recursive_mutex() : handle_(xSemaphoreCreateRecursiveMutex()) {
    ETL_ASSERT(handle_ != nullptr, std::bad_alloc());
  }

  /// @brief Destroy the recursive mutex and delete the underlying semaphore.
  ~recursive_mutex() {
    if (handle_ != nullptr) {
      vSemaphoreDelete(handle_);
    }
  }

  /// @brief Copy construction is disabled.
  recursive_mutex(const recursive_mutex& other) = delete;

  /// @brief Copy assignment is disabled.
  auto operator=(const recursive_mutex& other) -> recursive_mutex& = delete;

  /// @brief Move construction is disabled.
  recursive_mutex(recursive_mutex&& other) noexcept = delete;

  /// @brief Move assignment is disabled.
  auto operator=(recursive_mutex&& other) noexcept -> recursive_mutex& = delete;

  /// @brief Lock the recursive mutex, blocking until it can be acquired.
  void lock() { (void)xSemaphoreTakeRecursive(handle_, portMAX_DELAY); }

  /// @brief Try to lock the recursive mutex without blocking.
  /// @return true if the mutex was acquired, false otherwise.
  [[nodiscard]] auto try_lock() -> bool {
    return xSemaphoreTakeRecursive(handle_, 0) == pdTRUE;
  }

  /// @brief Unlock the recursive mutex.
  void unlock() { (void)xSemaphoreGiveRecursive(handle_); }

 private:
  SemaphoreHandle_t handle_;
};

}  // namespace paraos

#endif /* PARAOS_FREERTOS_RECURSIVE_MUTEX_HPP */
