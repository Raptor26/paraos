#ifndef PARAOS_MUTEX_HPP
#define PARAOS_MUTEX_HPP

#include <pthread.h>

#include "paraos_trace.hpp"
#include "paraos_utils.hpp"

namespace paraos {

struct MutexAttr {
  bool is_binary_ = false;
};

class MutexBase {
 public:
  MutexBase(const MutexAttr& attr) noexcept : is_binary_{attr.is_binary_} {
    if (!is_init_) {
      auto status = pthread_mutex_init(&m_obj_, nullptr);

      if (status == 0) {
        paraosTRACE_MESSAGE("Mutex constructed");
        is_init_ = true;
      }
    } else {
      assert(false && "Don't call second init for initalized object");
    }
  }

  MutexBase() noexcept : MutexBase(MutexAttr{}) {}

  virtual ~MutexBase() {
    pthread_mutex_destroy(&m_obj_);

    paraosTRACE_MESSAGE("Mutex deleted");
    // Debug only
    is_init_ = false;
  }

  /// @brief Возвращает true если мьютекс успешно инициализирован.
  /// @note Рекомендуется выполнить данную проверку единожды после создания
  /// мьютекса.
  operator bool() const noexcept { return is_init_; }

  MutexBase(const MutexBase& other) = delete;
  MutexBase(MutexBase&& other) = delete;

  MutexBase& operator=(const MutexBase& other) = delete;
  MutexBase& operator=(MutexBase&& other) = delete;

  virtual bool Lock(std::size_t timeout_ms = max_delay) noexcept {
    bool is_mutex_taken{false};

    int result = -1;
    if (timeout_ms == 0) {
      result = pthread_mutex_trylock(&m_obj_);
    } else {
      timespec delay{};
      delay.tv_nsec =
          static_cast<long>(timeout_ms) * NANOSECONDS_PER_MILISECONDS;

      timespec current_time{};
      clock_gettime(CLOCK_REALTIME, &current_time);

      TimespecAdd(&current_time, &delay, &delay);

      result = pthread_mutex_timedlock(&m_obj_, &delay);
    }

    if (result == 0) {
      is_mutex_taken = true;
      paraosTRACE_MESSAGE("Mutex locked");
    }

    return is_mutex_taken;
  }

  virtual bool Unlock() noexcept {
    bool is_mutex_released{false};

    auto result = pthread_mutex_unlock(&m_obj_);
    if (result == 0) {
      is_mutex_released = true;
      paraosTRACE_MESSAGE("Mutex unlocked");
    }

    return is_mutex_released;
  }

 private:
  pthread_mutex_t m_obj_;
  bool is_binary_{true};
  bool is_init_{false};
};
}  // namespace paraos

#endif /* PARAOS_MUTEX_HPP */
