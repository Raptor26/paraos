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

#include "FreeRTOS.h"
#include "paraos_utils.hpp"
#include "task.h"

namespace paraos {

/// @brief Return current tine in ticks. Useful when need periodical check
/// timeout in blocking operations with elapsed time correction.
///
/// @return Return object with current time. Returned value used in
/// CheckTimeout().
inline auto GetCurrentTime() noexcept {
  TimeOut_t xTimeOut;
  vTaskInternalSetTimeOutState(&xTimeOut);
  return xTimeOut;
}

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
/// @param[in] pxTimeOut: Returned by GetCurrentTime() value.
/// GetCurrentTimeInTicks() using at once before need periodical checking
/// timeout by CheckTimeout().
/// @param[in,out] ticks_to_wait: Wait time in ticks.
///
/// @return Return true if need break waiting, false if no timeout elapsed.
inline auto CheckTimeout(
    TimeOut_t &xTimeOut, paraos::delay_type &delay_ms) noexcept {
  // Conditions below useful in unit tests, because if scheduler not started,
  // xTaskCheckForTimeOut() catch segmentation fail.
  if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
    TickType_t ticks = PARAOS_ConvertMsToTicks(delay_ms);
    auto is_timeout = xTaskCheckForTimeOut(&xTimeOut, &ticks);
    delay_ms = PARAOS_ConvertTicksToMs(ticks);
    return static_cast<bool>(is_timeout);
  }

  return false;
}

}  // namespace paraos

#endif /* PARAOS_TIME_HPP */
