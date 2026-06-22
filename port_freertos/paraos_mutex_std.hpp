/// @file paraos_mutex_std.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_FREERTOS_MUTEX_STD_HPP
#define PARAOS_FREERTOS_MUTEX_STD_HPP

#include <new>

#include "FreeRTOS.h"
#include "etl/error_handler.h"
#include "semphr.h"

namespace paraos {

/// @brief std::mutex-style wrapper for FreeRTOS.
///
/// Provides lock(), try_lock(), and unlock() with semantics matching
/// std::mutex. The type is non-copyable and non-movable, and is compatible
/// with std::lock_guard and std::unique_lock.
class mutex {
 public:
  /// @brief Construct the mutex by creating a FreeRTOS mutex semaphore.
  mutex() : handle_(xSemaphoreCreateMutex()) {
    ETL_ASSERT(handle_ != nullptr, std::bad_alloc());
  }

  /// @brief Destroy the mutex and delete the underlying semaphore.
  ~mutex() {
    if (handle_ != nullptr) {
      vSemaphoreDelete(handle_);
    }
  }

  /// @brief Copy construction is disabled.
  mutex(const mutex& other) = delete;

  /// @brief Copy assignment is disabled.
  auto operator=(const mutex& other) -> mutex& = delete;

  /// @brief Move construction is disabled.
  mutex(mutex&& other) noexcept = delete;

  /// @brief Move assignment is disabled.
  auto operator=(mutex&& other) noexcept -> mutex& = delete;

  /// @brief Lock the mutex, blocking until it can be acquired.
  void lock() { (void)xSemaphoreTake(handle_, portMAX_DELAY); }

  /// @brief Try to lock the mutex without blocking.
  /// @return true if the mutex was acquired, false otherwise.
  [[nodiscard]] auto try_lock() -> bool {
    return xSemaphoreTake(handle_, 0) == pdTRUE;
  }

  /// @brief Unlock the mutex.
  void unlock() { (void)xSemaphoreGive(handle_); }

 private:
  SemaphoreHandle_t handle_;
};

}  // namespace paraos

#endif /* PARAOS_FREERTOS_MUTEX_STD_HPP */
