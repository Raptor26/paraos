#ifndef RTOS_IMPL_MUTEX_HPP
#define RTOS_IMPL_MUTEX_HPP

#if defined(WIN32)
#include "win/paraos_mutex.hpp"
#endif

#include "paraos_mutex.hpp"
#include "paraos_trace.hpp"

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

namespace paraos {
class MutexGuard {
 public:
  MutexGuard(MutexBase& mutex, std::size_t timeout_ms = max_delay)
      : mutex_{mutex} {
    mutex_.Lock(timeout_ms);
  }

  ~MutexGuard() { mutex_.Unlock(); }

  MutexGuard(const MutexGuard& other) = delete;
  MutexGuard(MutexGuard&& other) = delete;

  MutexGuard& operator=(const MutexGuard& other) = delete;
  MutexGuard& operator=(MutexGuard&& other) = delete;

 private:
  MutexBase& mutex_;
};
}  // namespace paraos

#endif /* RTOS_IMPL_MUTEX_HPP */
