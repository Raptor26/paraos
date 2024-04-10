#ifndef SEMAPHORE_HPP
#define SEMAPHORE_HPP

#include <stdio.h>

#include "paraos_utils.hpp"

namespace paraos {

struct SemaphoreAttr {
  std::size_t max_count = 1u;
};

constexpr std::size_t initial_count = 0u;

class Semaphore {
 public:
  Semaphore(const SemaphoreAttr attr)
      : handle_{
            CreateSemaphore(nullptr, initial_count, attr.max_count, nullptr)} {}
  Semaphore() : Semaphore{SemaphoreAttr{}} {}

  virtual ~Semaphore() {
    if (handle_) {
      CloseHandle(handle_);
    }
  }

  bool Take(std::size_t timeout_ms = max_delay) {
    bool is_sem_taken = false;
    DWORD signal_state =
        WaitForSingleObject(handle_, static_cast<DWORD>(timeout_ms));
    if (signal_state == WAIT_OBJECT_0) {
      is_sem_taken = true;
    }
    return is_sem_taken;
  }

  /// @brief
  /// @note
  /// https://learn.microsoft.com/ru-ru/windows/win32/api/synchapi/nf-synchapi-releasesemaphore
  /// @return
  bool Give() {
    constexpr LONG increment_sem_cnt{1u};

    return static_cast<bool>(
        ReleaseSemaphore(handle_, increment_sem_cnt, nullptr));
  }

  operator bool() const { return handle_ != nullptr ? true : false; }

 private:
  HANDLE handle_ = nullptr;
};

struct SemaphoreBinary final : public Semaphore {
  SemaphoreBinary() : Semaphore{} {}

  ~SemaphoreBinary() = default;
};

}  // namespace paraos

#endif /* SEMAPHORE_HPP */
