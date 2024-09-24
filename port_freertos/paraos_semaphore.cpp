/// @file paraos_semaphore.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author VyhodcevEgor <vyhodcev@internet.ru>
///
/// @copyright (c) 2024 Stilsoft
///
/// MIT License:
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the 'Software'), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.

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
    is_recursive_ = false;
  } else {
    handle_ = xSemaphoreCreateCounting(attr.max_count, initial_count);
    is_recursive_ = true;
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

ISRbool Semaphore::Take(std::size_t timeout_ms, bool from_isr) {
  PARAOS_CHECK_ASSERT(handle_);

  ISRbool status;
  BaseType_t xHigherPriorityTaskWoken{pdFALSE};

  if (!from_isr) {
    if (is_recursive_) {
      status.is_success_ =
          xSemaphoreTakeRecursive(handle_, PARAOS_ConvertMsToTicks(timeout_ms));
    } else {
      status.is_success_ =
          xSemaphoreTake(handle_, PARAOS_ConvertMsToTicks(timeout_ms));
    }
  } else {
    status.is_success_ =
        xSemaphoreTakeFromISR(handle_, &xHigherPriorityTaskWoken);
  }

  if (xHigherPriorityTaskWoken == pdTRUE) {
    status.is_need_switch_context_ = true;
  }

  return status;
}

ISRbool Semaphore::Give(bool from_isr) {
  PARAOS_CHECK_ASSERT(handle_);

  ISRbool status{};
  BaseType_t higher_priority_task_woken{pdFALSE};
  if (!from_isr) {
    if (is_recursive_) {
      status.is_success_ = xSemaphoreGiveRecursive(handle_);
    } else {
      status.is_success_ = xSemaphoreGive(handle_);
    }
  } else {
    status.is_success_ =
        xSemaphoreGiveFromISR(handle_, &higher_priority_task_woken);
  }

  if (higher_priority_task_woken == pdTRUE) {
    status.is_need_switch_context_ = true;
  }

  return status;
}

}  // namespace paraos
