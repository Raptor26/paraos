#ifndef RTOS_IMPL_MUTEX_HPP
#define RTOS_IMPL_MUTEX_HPP

#if defined(WIN32)
#include "win/mutex.hpp"
#endif

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

namespace paraos {
class MutexGuard {
 public:
  MutexGuard(MutexBase &mutex, std::size_t timeout_ms = max_delay)
      : mutex_{mutex} {
#ifdef paraosTRACE_ENABLE
    std::cout << "  MutexGuard Ctor" << std::endl;
#endif

    mutex_.Lock(timeout_ms);
  }

  ~MutexGuard() {
    mutex_.Unlock();
#ifdef paraosTRACE_ENABLE
    std::cout << "  MutexGuard Dtor" << std::endl;
#endif
  }

 private:
  const MutexBase &mutex_;
};
}  // namespace paraos

#endif /* RTOS_IMPL_MUTEX_HPP */
