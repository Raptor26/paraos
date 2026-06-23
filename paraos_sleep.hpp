/// @file paraos_sleep.hpp
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
///
/// @brief Cross-platform sleep helper for paraos.

#ifndef PARAOS_SLEEP_HPP
#define PARAOS_SLEEP_HPP

#include <chrono>
#include <thread>

#include "paraos_utils.hpp"

#ifdef PARAOS_LIKE_FREERTOS
#include "task.h"
#endif

namespace paraos {

/// @brief Suspend the calling thread/task for the given duration.
///
/// On PC/Unix/Windows this delegates to std::this_thread::sleep_for.
/// On FreeRTOS this delegates to vTaskDelay with tick conversion.
///
/// @param[in] duration Sleep duration in milliseconds.
inline void sleep_for(std::chrono::milliseconds duration) {
#ifdef PARAOS_LIKE_FREERTOS
  vTaskDelay(PARAOS_ConvertMsToTicks(
      static_cast<delay_type>(duration.count())));
#else
  std::this_thread::sleep_for(duration);
#endif
}

}  // namespace paraos

#endif  // PARAOS_SLEEP_HPP
