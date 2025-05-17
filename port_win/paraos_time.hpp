/// @file paraos_time.hpp
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

#ifndef PARAOS_TIME_HPP
#define PARAOS_TIME_HPP

#include <cstddef>

#include "paraos_critical.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_utils.hpp"

namespace paraos {

/// @brief Check timeout with elapsed time correction.
///
/// @details If a task enters and exits the Blocked state more than once while
/// it is waiting for the event to occur then the timeout used each time the
/// task enters the Blocked state must be adjusted to ensure the total of all
/// the time spent in the Blocked state does not exceed the originally
/// specified timeout period. xTaskCheckForTimeOut() performs the adjustment,
/// taking into account occasional occurrences such as tick count overflows,
/// which would otherwise make a manual adjustment prone to error.
///
/// @param[in] timeout: Returned by GetCurrentTime() value.
/// GetCurrentTimeInTicks() using at once before need periodical checking
/// timeout by CheckTimeout().
/// @param[in,out] delay_ms: Wait time in [ms]. Note: In windows port delay_ms
/// not modifed, by other ports (freeRTOS for example), delay_ms modify each
/// CheckTimeout() call.
///
/// @return Return true if need break waiting, false if no timeout elapsed.
inline auto CheckTimeout(OsProfiler &timeout, paraos::delay_type &delay_ms)
    -> bool {
  const CriticalSection critical;
  bool is_timeout{true};
  timeout.Stop();

  auto elapsed_time = timeout.LastDurationMs();

  if (delay_ms > elapsed_time) {
    is_timeout = false;

    // Reduced delay_ms. It's need for caller, which can again enter in
    // blocking mode with updated timeout.
    delay_ms -= elapsed_time;

    // Update start point because delay_ms was modified. It's necessary for
    // correct update delay_ms if CheckTimeout() will call again.
    timeout.Start();
  }

  return is_timeout;
}

/// @brief Return current time in ticks. Useful when need periodical check
/// timeout in blocking operations with elapsed time correction.
///
/// @return Return object with current time. Returned value used in
/// CheckTimeout().
inline auto GetCurrentTime() -> OsProfiler {
  // Create profiler and capture current time.
  OsProfiler profiler;
  profiler.Start();
  return profiler;
}

}  // namespace paraos

#endif /* PARAOS_TIME_HPP */
