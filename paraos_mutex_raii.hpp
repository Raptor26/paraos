/// @file paraos_mutex_raii.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_MUTEX_RAII_HPP
#define PARAOS_MUTEX_RAII_HPP

#include <mutex>

#include "paraos_mutex_std.hpp"

namespace paraos {

/// @brief RAII guard for paraos::mutex.
///
/// Provided for backward compatibility with code that previously used
/// MutexGuard with the legacy paraos::Mutex API. New code should prefer
/// std::scoped_lock<paraos::mutex> or std::unique_lock<paraos::mutex> directly.
using MutexGuard = std::scoped_lock<paraos::mutex>;

}  // namespace paraos

#endif /* PARAOS_MUTEX_RAII_HPP */
