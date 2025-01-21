/// @file paraos_utils.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author VyhodcevEgor <vyhodcev@internet.ru>
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

#ifndef PARAOS_UTILS_HPP
#define PARAOS_UTILS_HPP

#include <cstddef>
#include <limits>

#include "FreeRTOS.h"
#include "paraos_attr.h"

namespace paraos {

constexpr std::size_t max_delay{portMAX_DELAY};

constexpr std::size_t stack_multiplier{130};

inline auto PARAOS_ConvertMsToTicks(std::size_t uDelayInMs) -> TickType_t {
  static_assert(
      sizeof(uDelayInMs) >= sizeof(TickType_t),
      "uDelayInMs must be more or equal TickType_t size");

  if (uDelayInMs != portMAX_DELAY) {
    uDelayInMs = pdMS_TO_TICKS(uDelayInMs);
  }

  return (uDelayInMs);
}

inline auto PARAOS_ConvertTicksToMs(TickType_t ticks) -> std::size_t {
  std::size_t time_ms;
  if (ticks == portMAX_DELAY) {
    time_ms = std::numeric_limits<decltype(time_ms)>::max();
  } else {
    time_ms = pdTICKS_TO_MS(ticks);
  }

  return time_ms;
}

using FreeRTOSIdleFncPtr = void (*)();

inline FreeRTOSIdleFncPtr freertos_idle_fnc_ptr{nullptr};

constexpr auto GetStackMinimumSizeInBytes() -> std::size_t {
  return stack_multiplier * sizeof(size_t);
}

}  // namespace paraos

#endif /* PARAOS_UTILS_HPP */
