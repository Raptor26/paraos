/// @file paraos_recursive_mutex_std.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_RECURSIVE_MUTEX_STD_HPP
#define PARAOS_RECURSIVE_MUTEX_STD_HPP

#include <mutex>

namespace paraos {

/// @brief std::recursive_mutex-style wrapper for PC platforms.
///
/// Provides lock(), try_lock(), and unlock() with semantics matching
/// std::recursive_mutex. The type is non-copyable and non-movable, and is
/// compatible with std::lock_guard and std::unique_lock.
class recursive_mutex {
 public:
  /// @brief Default-construct the recursive mutex.
  recursive_mutex() = default;

  /// @brief Destroy the recursive mutex.
  ~recursive_mutex() = default;

  /// @brief Copy construction is disabled.
  recursive_mutex(const recursive_mutex& other) = delete;

  /// @brief Copy assignment is disabled.
  auto operator=(const recursive_mutex& other) -> recursive_mutex& = delete;

  /// @brief Move construction is disabled.
  recursive_mutex(recursive_mutex&& other) noexcept = delete;

  /// @brief Move assignment is disabled.
  auto operator=(recursive_mutex&& other) noexcept -> recursive_mutex& = delete;

  /// @brief Lock the recursive mutex, blocking until it can be acquired.
  void lock() { mutex_.lock(); }

  /// @brief Try to lock the recursive mutex without blocking.
  /// @return true if the mutex was acquired, false otherwise.
  [[nodiscard]] auto try_lock() -> bool { return mutex_.try_lock(); }

  /// @brief Unlock the recursive mutex.
  void unlock() { mutex_.unlock(); }

 private:
  std::recursive_mutex mutex_;
};

}  // namespace paraos

#endif /* PARAOS_RECURSIVE_MUTEX_STD_HPP */
