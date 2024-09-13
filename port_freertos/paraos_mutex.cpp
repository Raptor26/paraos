#include "paraos_mutex.hpp"

namespace paraos {

MutexBase::MutexBase(const MutexAttr& attr) : handle_{xSemaphoreCreateMutex()} {
  (void)attr;
#ifdef paraosTRACE_ENABLE
  std::cout << "RTOS MutexBase Ctor" << std::endl;
#endif
}

MutexBase::MutexBase() : MutexBase(MutexAttr{}) {}

MutexBase::~MutexBase() {
  PARAOS_CHECK_ASSERT(handle_ != nullptr);
  if (handle_) {
    vSemaphoreDelete(handle_);

    // need for debug only
    handle_ = nullptr;
  }

#ifdef paraosTRACE_ENABLE
  std::cout << "RTOS MutexBase Dtor" << std::endl;
#endif
}

MutexBase::operator bool() const { return handle_ != nullptr ? true : false; }

bool MutexBase::Lock(std::size_t timeout_ms) {
  auto result = xSemaphoreTake(
      handle_, static_cast<TickType_t>(PARAOS_ConvertMsToTicks(timeout_ms)));
  return static_cast<bool>(result);
}

bool MutexBase::Unlock() { return static_cast<bool>(xSemaphoreGive(handle_)); }

MutexBaseBinary::MutexBaseBinary() : MutexBase(MutexAttr{true}) {}

MutexBaseBinary::~MutexBaseBinary() {}

bool MutexBaseBinary::Lock(std::size_t timeout_ms) {
  bool is_current_operation_locked{false};
  if (is_locked_ == false) {
    is_current_operation_locked = MutexBase::Lock(timeout_ms);
    // We call MutexBase::Lock() if our current state "unlocked". If
    // MutexBase::Lock() returned false (from unlocked state), i don't know
    // what that mean. Try find race condition for "is_locked_" variable in
    // "MutexBaseBinary" class.
    PARAOS_CHECK_ASSERT(is_current_operation_locked == true);
    is_locked_ = true;
  }

  return is_current_operation_locked;
}

bool MutexBaseBinary::Unlock() {
  bool is_current_operation_unlocked{false};
  if (is_locked_ == true) {
    is_current_operation_unlocked = MutexBase::Unlock();
    // We call MutexBase::Unlock() if our current state "locked". If
    // MutexBase::Unlock() returned false (from "locked" state), i don't know
    // what that mean. Try find race condition for "is_locked_" variable in
    // "MutexBaseBinary" class.
    PARAOS_CHECK_ASSERT(is_current_operation_unlocked == true);
    is_locked_ = false;
  }

  return is_current_operation_unlocked;
}

RecursiveMutex::RecursiveMutex(const MutexAttr& attr)
    : handle_{xSemaphoreCreateRecursiveMutex()} {
  (void)attr;
#ifdef paraosTRACE_ENABLE
  std::cout << "RecursiveMutex Ctor" << std::endl;
#endif
}

RecursiveMutex::RecursiveMutex() : RecursiveMutex(MutexAttr{}) {}

RecursiveMutex::~RecursiveMutex() {
  PARAOS_CHECK_ASSERT(handle_ != nullptr);
  if (handle_) {
    vSemaphoreDelete(handle_);

    // need for debug only
    handle_ = nullptr;
  }

#ifdef paraosTRACE_ENABLE
  std::cout << "RecursiveMutex Dtor" << std::endl;
#endif
}

RecursiveMutex::operator bool() const {
  return handle_ != nullptr ? true : false;
}

bool RecursiveMutex::Lock(std::size_t timeout_ms) {
  return static_cast<bool>(xSemaphoreTakeRecursive(
      handle_, static_cast<TickType_t>(PARAOS_ConvertMsToTicks(timeout_ms))));
}

bool RecursiveMutex::Unlock() {
  return static_cast<bool>(xSemaphoreGiveRecursive(handle_));
}

}  // namespace paraos