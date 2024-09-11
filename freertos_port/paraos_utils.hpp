#ifndef PARAOS_UTILS_HPP
#define PARAOS_UTILS_HPP

#include <cstddef>

#include "FreeRTOS.h"
#include "irtos_attr.h"

namespace paraos {

constexpr std::size_t max_delay{portMAX_DELAY};

extern "C" inline std::size_t RTOS_THREAD_ConvertMsToTicks(
    std::size_t uDelayInMs) {
  static_assert(
      sizeof(uDelayInMs) >= sizeof(TickType_t),
      "uDelayInMs must be more or equal TickType_t size");

  if (uDelayInMs != portMAX_DELAY) {
    uDelayInMs = pdMS_TO_TICKS(uDelayInMs);
  }

  return (uDelayInMs);
}

}  // namespace paraos

#endif /* PARAOS_UTILS_HPP */
