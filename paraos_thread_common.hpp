/// @file paraos_thread_common.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_THREAD_COMMON_HPP
#define PARAOS_THREAD_COMMON_HPP

#include <cstddef>
#include <string_view>

#include "etl/delegate.h"
#include "paraos_attr.h"
#include "paraos_utils.hpp"

#ifdef PARAOS_LIKE_WINAPI
#include <winbase.h>
#endif

namespace paraos {

/// @brief Delegate type.
using thread_delegate_type = etl::delegate<void()>;

#ifdef PARAOS_LIKE_WINAPI
enum class thread_priority : int8_t {
  idle = THREAD_PRIORITY_IDLE,
  lowest = THREAD_PRIORITY_LOWEST,
  below_normal = THREAD_PRIORITY_BELOW_NORMAL,
  normal = THREAD_PRIORITY_NORMAL,
  above_normal = THREAD_PRIORITY_ABOVE_NORMAL,
  highest = THREAD_PRIORITY_HIGHEST,
  realtime = THREAD_PRIORITY_TIME_CRITICAL,

  // Backward-compatible aliases for old enum value names.
  kIdle = idle,
  kLowest = lowest,
  kBelowNormal = below_normal,
  kNormal = normal,
  kAboveNormal = above_normal,
  kHighest = highest,
  kRealTime = realtime,
};
#elif defined(PARAOS_LIKE_FREERTOS)
enum class thread_priority : uint8_t {
  idle = 0,
  lowest = 1,
  below_normal = 2,
  normal = 3,
  above_normal = 4,
  highest = 5,
  realtime = 6,

  // Backward-compatible aliases for old enum value names.
  kIdle = idle,
  kLowest = lowest,
  kBelowNormal = below_normal,
  kNormal = normal,
  kAboveNormal = above_normal,
  kHighest = highest,
  kRealTime = realtime,

  max_num = 7
};

#elif defined(PARAOS_LIKE_UNIX)
enum class thread_priority : uint8_t {
  idle = 1,
  lowest = 2,
  below_normal = 3,
  normal = 4,
  above_normal = 5,
  highest = 6,
  realtime = 7,

  // Backward-compatible aliases for old enum value names.
  kIdle = idle,
  kLowest = lowest,
  kBelowNormal = below_normal,
  kNormal = normal,
  kAboveNormal = above_normal,
  kHighest = highest,
  kRealTime = realtime,
};
#endif

using ThreadPriority PARAOS_DEPRECATED("use paraos::thread_priority") =
    thread_priority;

/// @brief Parameters to initialize paraos::jthread.
struct thread_attr {
  /// @brief The name of the thread being created.
  std::string_view thread_name{"Thread"};

  /// @brief Stack depth in bytes.
  /// Must be greater than or equal to paraos::get_stack_minimum_size_in_bytes().
  std::size_t stack_depth{paraos::get_stack_minimum_size_in_bytes()};

  /// @brief The priority of the thread being created.
  paraos::thread_priority priority{paraos::thread_priority::normal};
};

using ThreadAttr PARAOS_DEPRECATED("use paraos::thread_attr") = thread_attr;

}  // namespace paraos

#endif /* PARAOS_THREAD_COMMON_HPP */
