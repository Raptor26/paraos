#ifndef PARAOS_UTILS_HPP
#define PARAOS_UTILS_HPP
#include <cassert>
#include <cstddef>

namespace paraos {

constexpr std::size_t max_delay{1000};
// static_assert(sizeof(max_delay) >= sizeof(DWORD));

#define MICROSECONDS_PER_SECOND (1000000LL)   /**< Microseconds per second. */
#define NANOSECONDS_PER_SECOND (1000000000LL) /**< Nanoseconds per second. */
#define NANOSECONDS_PER_MILISECONDS \
  (1000000LL) /**< Nanoseconds per microseconds. */

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

}  // namespace paraos

#endif /* PARAOS_UTILS_HPP */
