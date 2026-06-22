/// @file paraos_mutex_std.hpp
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
/// @brief PC implementation of paraos::mutex as a thin wrapper over std::mutex.

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
