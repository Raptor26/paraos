#ifndef MUTEX_HPP
#define MUTEX_HPP

#include "paraos_utils.hpp"

namespace paraos {

struct MutexAttr {
  bool is_binary_ = true;
};

/// @brief
/// @note Пример использования мьютексов можно найти по ссылке ниже
/// https://learn.microsoft.com/ru-ru/windows/win32/sync/using-mutex-objects
class MutexBase final {
 public:
  MutexBase(const MutexAttr &attr) : is_binary_{attr.is_binary_} {
    handle_ = CreateMutex(nullptr, false, nullptr);
  }

  MutexBase() : MutexBase(MutexAttr{}) {}

  ~MutexBase() {
    if (handle_) {
      CloseHandle(handle_);
    }
  }

  operator bool() const { return handle_ != nullptr ? true : false; }

  bool Lock(std::size_t timeout_ms = max_delay) const {
    bool is_mutex_taken = false;
    DWORD signal_state =
        WaitForSingleObject(handle_, static_cast<DWORD>(timeout_ms));
    if (signal_state == WAIT_OBJECT_0) {
      is_mutex_taken = true;
    }
    return is_mutex_taken;
  }

  bool Unlock() const { return static_cast<bool>(ReleaseMutex(handle_)); }

 private:
  HANDLE handle_ = nullptr;
  bool is_binary_ = true;
};
}  // namespace paraos

#endif /* MUTEX_HPP */
