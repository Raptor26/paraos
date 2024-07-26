#ifndef PARAOS_SEMAPHORE_HPP
#define PARAOS_SEMAPHORE_HPP

#include <pthread.h>
#include <semaphore.h>

#include "paraos_utils.hpp"

namespace paraos {

struct SemaphoreAttr {
  std::size_t max_count{1u};
  std::size_t initial_value{0};
};

constexpr std::size_t initial_count = 0u;

class Semaphore {
 public:
  Semaphore(const SemaphoreAttr attr) {
    sem_init(&handle_, 0, attr.initial_value);
  }

  Semaphore() : Semaphore{SemaphoreAttr{}} {}

  virtual ~Semaphore() { sem_close(&handle_); }

  bool Take(std::size_t timeout_ms = max_delay) {
    bool is_sem_taken = false;
    int result = -1;
    if (timeout_ms == 0) {
      result = sem_trywait(&handle_);
    } else if (timeout_ms == max_delay) {
      result = sem_wait(&handle_);
    } else {
      timespec delay{};
      delay.tv_nsec =
          static_cast<long>(timeout_ms) * NANOSECONDS_PER_MILISECONDS;

      timespec current_time{};
      clock_gettime(CLOCK_REALTIME, &current_time);

      TimespecAdd(&current_time, &delay, &delay);

      result = sem_timedwait(&handle_, &delay);
    }

    if (result == 0) {
      is_sem_taken = true;
    }
    return is_sem_taken;
  }

  bool Give(bool from_isr = false) {
    bool is_sem_given{false};
    auto result = sem_post(&handle_);

    if (result == 0) {
      is_sem_given = true;
    }
    return is_sem_given;
  }

  operator bool() const { return false; }

 private:
  sem_t handle_;
};

struct SemaphoreBinary final : public Semaphore {
  SemaphoreBinary() : Semaphore{} {}

  ~SemaphoreBinary() = default;
};
}  // namespace paraos

#endif /* PARAOS_SEMAPHORE_HPP */
