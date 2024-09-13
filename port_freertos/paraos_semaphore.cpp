#include "paraos_semaphore.hpp"

#include "paraos_check.h"

namespace paraos {

Semaphore::Semaphore(const SemaphoreAttr attr) noexcept {
#ifdef paraosTRACE_ENABLE
  std::cout << "Semaphore Ctor" << std::endl;
#endif
  // В зависимости от максимального значения счётчика создаваемого семафора
  // будет создан либо бинарный, либо счётный семафор.
  if (attr.max_count < 2) {
    handle_ = xSemaphoreCreateBinary();
  } else {
    handle_ = xSemaphoreCreateCounting(attr.max_count, initial_count);
  }
}

Semaphore::~Semaphore() {
  if (handle_) {
    vSemaphoreDelete(handle_);

    // need for debug only
    handle_ = nullptr;
  }

#ifdef paraosTRACE_ENABLE
  std::cout << "Semaphore Dtor" << std::endl;
#endif
}

Semaphore::operator bool() const { return handle_ != nullptr ? true : false; }

bool Semaphore::Take(std::size_t timeout_ms) {
  PARAOS_CHECK_ASSERT(handle_ != nullptr);
  return static_cast<bool>(
      xSemaphoreTake(handle_, RTOS_THREAD_ConvertMsToTicks(timeout_ms)));
}

bool Semaphore::Give(bool from_isr) {
  PARAOS_CHECK_ASSERT(handle_ != nullptr);

  auto success = pdTRUE;
  if (from_isr == true) {
    BaseType_t higher_priority_task_woken = 1;
    success = xSemaphoreGiveFromISR(handle_, &higher_priority_task_woken);
  } else {
    success = xSemaphoreGive(handle_);
  }

  return success == pdTRUE ? true : false;
}

}  // namespace paraos
