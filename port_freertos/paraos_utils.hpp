#ifndef PARAOS_UTILS_HPP
#define PARAOS_UTILS_HPP

#include <cstddef>

#include "FreeRTOS.h"
#include "paraos_attr.h"

namespace paraos {

constexpr std::size_t max_delay{portMAX_DELAY};

inline std::size_t PARAOS_ConvertMsToTicks(std::size_t uDelayInMs) {
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
