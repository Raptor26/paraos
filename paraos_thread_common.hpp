/// @file paraos_thread_common.hpp
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

#ifndef PARAOS_THREAD_COMMON_HPP
#define PARAOS_THREAD_COMMON_HPP

#include <cstddef>
#include <string_view>

#include "etl/delegate.h"
#include "paraos_attr.h"
#include "paraos_base.hpp"

#if defined(PARAOS_LIKE_WINAPI)
#include <winbase.h>
#endif

namespace paraos {
namespace v2 {

/// @brief Delegate type.
using thread_delegate_type = etl::delegate<void()>;

#if defined(PARAOS_LIKE_WINAPI)
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

/// @brief Parameters to initialize paraos::Thread.
struct ThreadAttr {
  /// @brief The name of the thread being created.
  std::string_view thread_name{"Thread"};

  /// @brief Stack depth in bytes.
  /// Must be greater than or equal to paraos::GetStackMinimumSizeInBytes().
  std::size_t stack_depth{paraos::GetStackMinimumSizeInBytes()};

  /// @brief The priority of the thread being created.
  paraos::v2::ThreadPriority priority{paraos::v2::ThreadPriority::kNormal};

  /// @brief The user can set a callback to be called when the destructor
  /// of a paraos::Base object is invoked.
  base_callback dtor_callback{nullptr};

  /// @brief Run this delegate in thread context. User code will can register
  /// delegate later.
  ///
  /// @see https://www.etlcpp.com/delegate.html to delegate creation examples.
  thread_delegate_type run_;
};

}  // namespace v2
}  // namespace paraos

#endif /* PARAOS_THREAD_COMMON_HPP */
