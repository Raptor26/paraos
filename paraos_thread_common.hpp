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
enum ThreadPriority : int8_t {
  kIdle = THREAD_PRIORITY_IDLE,
  kLowest = THREAD_PRIORITY_LOWEST,
  kBelowNormal = THREAD_PRIORITY_BELOW_NORMAL,
  kNormal = THREAD_PRIORITY_NORMAL,
  kAboveNormal = THREAD_PRIORITY_ABOVE_NORMAL,
  kHighest = THREAD_PRIORITY_HIGHEST,
  kRealTime = THREAD_PRIORITY_TIME_CRITICAL,
};
#elif defined(PARAOS_LIKE_FREERTOS)
enum class ThreadPriority : uint8_t {
  kIdle = 0,
  kLowest,
  kBelowNormal,
  kNormal,
  kAboveNormal,
  kHighest,
  kRealTime,

  kMaxNum
};

#elif defined(PARAOS_LIKE_UNIX)
enum class ThreadPriority : uint8_t {
  kIdle = 1,
  kLowest,
  kBelowNormal,
  kNormal,
  kAboveNormal,
  kHighest,
  kRealTime,
};
#endif

/// @brief Parameters to initialize paraos::jthread.
struct ThreadAttr {
  /// @brief The name of the thread being created.
  std::string_view thread_name{"Thread"};

  /// @brief Stack depth in bytes.
  /// Must be greater than or equal to paraos::GetStackMinimumSizeInBytes().
  std::size_t stack_depth{paraos::GetStackMinimumSizeInBytes()};

  /// @brief The priority of the thread being created.
  paraos::ThreadPriority priority{paraos::ThreadPriority::kNormal};
};

}  // namespace paraos

#endif /* PARAOS_THREAD_COMMON_HPP */
