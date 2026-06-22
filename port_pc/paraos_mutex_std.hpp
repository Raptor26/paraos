/// @file paraos_mutex_std.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_MUTEX_STD_HPP
#define PARAOS_MUTEX_STD_HPP

#include <mutex>

namespace paraos {

/// @brief std::mutex-style wrapper for PC platforms.
///
/// Provides lock(), try_lock(), and unlock() with semantics matching
/// std::mutex. The type is non-copyable and non-movable, and is compatible
/// with std::lock_guard and std::unique_lock.
class mutex {
 public:
  /// @brief Default-construct the mutex.
  mutex() = default;

  /// @brief Destroy the mutex.
  ~mutex() = default;

  /// @brief Copy construction is disabled.
  mutex(const mutex& other) = delete;

  /// @brief Copy assignment is disabled.
  auto operator=(const mutex& other) -> mutex& = delete;

  /// @brief Move construction is disabled.
  mutex(mutex&& other) noexcept = delete;

  /// @brief Move assignment is disabled.
  auto operator=(mutex&& other) noexcept -> mutex& = delete;

  /// @brief Lock the mutex, blocking until it can be acquired.
  void lock() { mutex_.lock(); }

  /// @brief Try to lock the mutex without blocking.
  /// @return true if the mutex was acquired, false otherwise.
  [[nodiscard]] auto try_lock() -> bool { return mutex_.try_lock(); }

  /// @brief Unlock the mutex.
  void unlock() { mutex_.unlock(); }

 private:
  std::mutex mutex_;
};

}  // namespace paraos

#endif /* PARAOS_MUTEX_STD_HPP */
