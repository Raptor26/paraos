#ifndef RTOS_IMPL_MUTEX_HPP
#define RTOS_IMPL_MUTEX_HPP

#if defined(WIN32)
#include "win/mutex.hpp"
#endif

namespace paraos {
class MutexGuard {
 public:
  MutexGuard(MutexBase &mutex, std::size_t timeout_ms = max_delay)
      : mutex_{mutex} {
    mutex_.Lock(timeout_ms);
  }

  ~MutexGuard() { mutex_.Unlock(); }

 private:
  const MutexBase &mutex_;
};
}  // namespace paraos

#endif /* RTOS_IMPL_MUTEX_HPP */
