/// @file paraos_utils.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_UTILS_HPP
#define PARAOS_UTILS_HPP

#include <time.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace paraos {

constexpr std::size_t max_delay{std::numeric_limits<std::size_t>::max()};

constexpr std::size_t stack_multiplier{1024};

#define MICROSECONDS_PER_SECOND (1000000LL)   /**< Microseconds per second. */
#define NANOSECONDS_PER_SECOND (1000000000LL) /**< Nanoseconds per second. */
#define NANOSECONDS_PER_MILISECONDS \
  (1000000LL) /**< Nanoseconds per miliseconds. */
#define MILISECONDS_PER_SECOND (1000LL)

#define MICROSECONDS_PER_MILISECONDS (1000LL)

inline auto TimespecAdd(
    const struct timespec* const first, const struct timespec* const second,
    struct timespec* const pxResult) -> int {
  std::int64_t llPartialSec = 0;
  int iStatus = 0;

  /* Check parameters. */
  if ((pxResult == nullptr) || (first == nullptr) || (second == nullptr)) {
    iStatus = -1;
  }

  if (iStatus == 0) {
    /* Perform addition. */
    pxResult->tv_nsec = first->tv_nsec + second->tv_nsec;

    /* check for overflow in case nsec value was invalid */
    if (pxResult->tv_nsec < 0) {
      iStatus = 1;
    } else {
      llPartialSec = (pxResult->tv_nsec) / NANOSECONDS_PER_SECOND;
      pxResult->tv_nsec = (pxResult->tv_nsec) % NANOSECONDS_PER_SECOND;
      pxResult->tv_sec = first->tv_sec + second->tv_sec + llPartialSec;

      /* check for overflow */
      if (pxResult->tv_sec < 0) {
        iStatus = 1;
      }
    }
  }

  return iStatus;
}

constexpr auto GetStackMinimumSizeInBytes() -> std::size_t {
  return stack_multiplier * sizeof(size_t);
}

/// @brief Calculate period in <timespec> class from time in milliseconds.
/// @param[in] milliseconds: Time in milliseconds for convert in timespec class.
/// @return struct timespec with filled fields.
inline auto MillisecondsInTimeSpec(std::size_t milliseconds)
    -> struct timespec {
  struct timespec tspec {};
  tspec.tv_sec = milliseconds / MILISECONDS_PER_SECOND;
  tspec.tv_nsec =
      (milliseconds % MILISECONDS_PER_SECOND) * MICROSECONDS_PER_SECOND;
  return tspec;
}

using delay_type = std::size_t;

}  // namespace paraos

#endif /* PARAOS_UTILS_HPP */
