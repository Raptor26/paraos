/// @file paraos_utils.hpp
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

#ifndef PARAOS_UTILS_HPP
#define PARAOS_UTILS_HPP

#include <sys/time.h>

#include <cassert>
#include <cstddef>
#include <limits>

namespace paraos {

constexpr std::size_t max_delay{std::numeric_limits<std::size_t>::max()};

#define MICROSECONDS_PER_SECOND (1000000LL)   /**< Microseconds per second. */
#define NANOSECONDS_PER_SECOND (1000000000LL) /**< Nanoseconds per second. */
#define NANOSECONDS_PER_MILISECONDS \
  (1000000LL) /**< Nanoseconds per microseconds. */
#define MILISECONDS_PER_SECOND (1000LL)

#define MICROSECONDS_PER_MILISECONDS (1000LL)

inline int TimespecAdd(
    const struct timespec* const x, const struct timespec* const y,
    struct timespec* const pxResult) {
  int64_t llPartialSec = 0;
  int iStatus = 0;

  /* Check parameters. */
  if ((pxResult == nullptr) || (x == nullptr) || (y == nullptr)) {
    iStatus = -1;
  }

  if (iStatus == 0) {
    /* Perform addition. */
    pxResult->tv_nsec = x->tv_nsec + y->tv_nsec;

    /* check for overflow in case nsec value was invalid */
    if (pxResult->tv_nsec < 0) {
      iStatus = 1;
    } else {
      llPartialSec = (pxResult->tv_nsec) / NANOSECONDS_PER_SECOND;
      pxResult->tv_nsec = (pxResult->tv_nsec) % NANOSECONDS_PER_SECOND;
      pxResult->tv_sec = x->tv_sec + y->tv_sec + llPartialSec;

      /* check for overflow */
      if (pxResult->tv_sec < 0) {
        iStatus = 1;
      }
    }
  }

  return iStatus;
}

constexpr inline std::size_t GetStackMinimumSizeInBytes() {
  return 1024 * sizeof(size_t);
}

/// @brief Calculate period in <timespec> class from time in milliseconds.
/// @param[in] milliseconds: Time in milliseconds for convert in timespec class.
/// @return struct timespec with filled fields.
inline struct timespec MillisecondsInTimeSpec(std::size_t milliseconds) {
  struct timespec time_y_milliseconds {};
  time_y_milliseconds.tv_sec = static_cast<time_t>(milliseconds) /
                        static_cast<time_t>(MILISECONDS_PER_SECOND);

  time_t ms = milliseconds - (time_y_milliseconds.tv_sec * MILISECONDS_PER_SECOND);
  time_y_milliseconds.tv_nsec = ms * NANOSECONDS_PER_MILISECONDS;

  return time_y_milliseconds;
}

}  // namespace paraos

#endif /* PARAOS_UTILS_HPP */
