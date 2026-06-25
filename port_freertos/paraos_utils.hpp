/// @file paraos_utils.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_UTILS_HPP
#define PARAOS_UTILS_HPP

#include <cstddef>
#include <limits>

#include "FreeRTOS.h"
#include "paraos_attr.h"
#include "paraos_config.hpp"

namespace paraos {

using delay_type = TickType_t;

constexpr delay_type max_delay{portMAX_DELAY};

constexpr std::size_t machine_world_len = sizeof(std::size_t);

inline auto PARAOS_ConvertMsToTicks(delay_type uDelayInMs) noexcept {
  static_assert(
      sizeof(uDelayInMs) >= sizeof(TickType_t),
      "uDelayInMs must be more or equal TickType_t size");

  if (uDelayInMs != max_delay) {
    uDelayInMs = pdMS_TO_TICKS(uDelayInMs);
  }

  return static_cast<TickType_t>(uDelayInMs);
}

inline auto PARAOS_ConvertTicksToMs(TickType_t ticks) noexcept {
  delay_type time_ms{max_delay};
  if (ticks != portMAX_DELAY) {
    time_ms = pdTICKS_TO_MS(ticks);
  }

  return time_ms;
}

using FreeRTOSIdleFncPtr = void (*)();

inline FreeRTOSIdleFncPtr freertos_idle_fnc_ptr{nullptr};

constexpr auto get_stack_minimum_size_in_bytes() -> std::size_t {
  return configMINIMAL_STACK_SIZE_IN_BYTES * machine_world_len;
}

constexpr auto convert_stack_size_in_words(std::size_t stack_size_in_bytes) {
  return stack_size_in_bytes / machine_world_len;
}

PARAOS_DEPRECATED("use get_stack_minimum_size_in_bytes()")
constexpr auto GetStackMinimumSizeInBytes() -> std::size_t {
  return get_stack_minimum_size_in_bytes();
}

PARAOS_DEPRECATED("use convert_stack_size_in_words()")
constexpr auto ConvertStackSizeInWords(std::size_t stack_size_in_bytes) {
  return convert_stack_size_in_words(stack_size_in_bytes);
}

}  // namespace paraos

#endif /* PARAOS_UTILS_HPP */
