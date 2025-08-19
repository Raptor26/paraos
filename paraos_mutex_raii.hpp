/// @file paraos_mutex_raii.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2024 Stilsoft
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
