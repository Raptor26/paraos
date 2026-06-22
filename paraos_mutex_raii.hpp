/// @file paraos_mutex_raii.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_MUTEX_RAII_HPP
#define PARAOS_MUTEX_RAII_HPP

#include "paraos_mutex.hpp"
#include "paraos_trace.hpp"

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

namespace paraos {
class MutexGuard {
 public:
  explicit MutexGuard(MutexBase& mutex, std::size_t timeout_ms = max_delay)
      : mutex_{mutex}, is_locked{mutex_.Lock(timeout_ms)} {}

  ~MutexGuard() { mutex_.Unlock(); }

  MutexGuard(const MutexGuard& other) = delete;
  MutexGuard(MutexGuard&& other) = delete;

  auto operator=(const MutexGuard& other) -> MutexGuard& = delete;
  auto operator=(MutexGuard&& other) -> MutexGuard& = delete;

  [[nodiscard]] auto IsLocked() const { return is_locked; }

 private:
  MutexBase& mutex_;
  const bool is_locked{};
};
}  // namespace paraos

#endif /* PARAOS_MUTEX_RAII_HPP */
