#ifndef MUTEX_HPP
#define MUTEX_HPP

#include "paraos_utils.hpp"

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

namespace paraos {

struct MutexAttr {
  bool is_binary_ = true;
};

/// @brief
/// @note Пример использования мьютексов можно найти по ссылке ниже
/// https://learn.microsoft.com/ru-ru/windows/win32/sync/using-mutex-objects
class MutexBase final {
 public:
  MutexBase(const MutexAttr &attr)
      : handle_{CreateMutex(nullptr, false, nullptr)},
        is_binary_{attr.is_binary_} {
#ifdef paraosTRACE_ENABLE
    std::cout << "MutexBase Ctor" << std::endl;
#endif
  }

  MutexBase() : MutexBase(MutexAttr{}) {}

  ~MutexBase() {
    assert(handle_);
    if (handle_) {
      CloseHandle(handle_);

      // need for debug only
      handle_ = nullptr;
    }

#ifdef paraosTRACE_ENABLE
    std::cout << "MutexBase Dtor" << std::endl;
#endif
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
