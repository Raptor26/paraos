/// @file paraos_semaphore.hpp
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

#ifndef PARAOS_SEMAPHORE_HPP
#define PARAOS_SEMAPHORE_HPP

#include <stddef.h>

#include <utility>

#include "FreeRTOS.h"
#include "paraos_check.h"
#include "paraos_isr.hpp"
#include "paraos_utils.hpp"
#include "semphr.h"

namespace paraos {

/// @brief Аттрибуты семафора, передаваемые ему при инициализации.
struct SemaphoreAttr {
  std::size_t max_count{1u};
  std::size_t initial_count{0u};
};

constexpr std::size_t initial_count = 0u;

/// @brief Base semaphore class. Provides Take and Give operations. Designed for
/// inheritance only, for example, by classes such as SemaphoreCounting and
/// SemaphoreBinary.
class SemaphoreBase {
 public:
  operator bool() const noexcept { return handle_ != nullptr ? true : false; }

  /// @brief Take Semaphore.
  ///
  /// @param[in] timeout_ms: If nothing given semaphore, Take() will wait while
  /// anything Give() semaphore with timeout, set in timeout_ms variable.
  ///
  /// @param[in] from_isr: Set true, if called from isr.
  ///
  /// @return Return true if semaphore was taken under timeout, false in
  /// otherwise.
  ISRbool Take(
      std::size_t timeout_ms = max_delay, bool from_isr = false) noexcept {
    PARAOS_CHECK_ASSERT(handle_);

    ISRbool status;

    if (!from_isr) {
      // API for semaphore and recursive semaphore taking are identically!!!
      // Not need call xSemaphoreTakeRecursive().
      // xSemaphoreTake() may used with SemaphoreCounting and SemaphoreBinary
      // classes.
      status.is_success_ =
          xSemaphoreTake(handle_, PARAOS_ConvertMsToTicks(timeout_ms));
    } else {
      BaseType_t xHigherPriorityTaskWoken{pdFALSE};

      // xSemaphoreTakeFromISR() may used with SemaphoreCounting and
      // SemaphoreBinary classes.
      status.is_success_ =
          xSemaphoreTakeFromISR(handle_, &xHigherPriorityTaskWoken);

      if (xHigherPriorityTaskWoken == pdTRUE) {
        status.is_need_switch_context_ = true;
      }
    }

    return status;
  }

  /// @brief Release semaphore.
  ///
  /// @param[in] from_isr: Set true, if called from isr.
  ///
  /// @return Return operation status. ISRbool contained value indicate is need
  /// switch context. Useful when Give() called from isr.
  ISRbool Give(bool from_isr = false) noexcept {
    PARAOS_CHECK_ASSERT(handle_);

    ISRbool status;

    if (!from_isr) {
      // API for semaphore and recursive semaphore giving are identically!!!
      // Not need call xSemaphoreGiveRecursive().
      // xSemaphoreGive() may used with SemaphoreCounting and SemaphoreBinary
      // classes.
      status.is_success_ = xSemaphoreGive(handle_);
    } else {
      BaseType_t higher_priority_task_woken{pdFALSE};

      // xSemaphoreGiveFromISR() may used with SemaphoreCounting and
      // SemaphoreBinary classes.
      status.is_success_ =
          xSemaphoreGiveFromISR(handle_, &higher_priority_task_woken);

      if (higher_priority_task_woken == pdTRUE) {
        status.is_need_switch_context_ = true;
      }
    }

    return status;
  }

 protected:
  SemaphoreBase() = default;
  virtual ~SemaphoreBase() {
    if (handle_) {
      vSemaphoreDelete(handle_);
      handle_ = nullptr;
    }
  }

  /// @brief Move ctor.
  SemaphoreBase(SemaphoreBase &&other) {
    if (this != &other) {
      this->handle_ = other.handle_;
      other.handle_ = nullptr;
    }
  }

  /// @brief Move assignment.
  SemaphoreBase &operator=(SemaphoreBase &&other) {
    if (this != &other) {
      this->~SemaphoreBase();
      this->handle_ = other.handle_;
      other.handle_ = nullptr;
    }

    return *this;
  }

  /// @brief Semaphore non-copyable
  SemaphoreBase(const SemaphoreBase &other) = delete;
  SemaphoreBase &operator=(const SemaphoreBase &other) = delete;

  SemaphoreHandle_t handle_;
};

/// @brief Counting semaphore. Max call Give() determine in attr.max_count.
struct SemaphoreCounting final : public SemaphoreBase {
  SemaphoreCounting(const SemaphoreAttr &attr) noexcept : SemaphoreBase{} {
    handle_ = xSemaphoreCreateCounting(attr.max_count, attr.initial_count);
  }

  /// @brief Semaphore deleted by ~SemaphoreBase()
  ~SemaphoreCounting() = default;

  /// @brief Move ctor.
  SemaphoreCounting(SemaphoreCounting &&other)
      : SemaphoreBase(std::move(other)) {}

  /// @brief Move assignment.
  SemaphoreCounting &operator=(SemaphoreCounting &&other) {
    if (this != &other) {
      this->~SemaphoreCounting();
      this->handle_ = other.handle_;
      other.handle_ = nullptr;
    }

    return *this;
  }

  /// @brief Semaphore non-copyable
  SemaphoreCounting(const SemaphoreCounting &other) = delete;
  SemaphoreCounting &operator=(const SemaphoreCounting &other) = delete;
};

/// @brief The binary semaphore is created in the 'empty' state,
/// meaning the semaphore must first be given using the Give() API
/// function before it can subsequently be taken (obtained) using the
/// Take() function.
///
/// @note A binary semaphore need not be given back once obtained, so task
/// synchronisation can be implemented by one task/interrupt continuously
/// 'giving' the semaphore while another continuously 'takes' the semaphore.
struct SemaphoreBinary final : public SemaphoreBase {
  SemaphoreBinary() noexcept : SemaphoreBinary{SemaphoreAttr{}} {}

  SemaphoreBinary(const SemaphoreAttr &attr) noexcept : SemaphoreBase{} {
    handle_ = xSemaphoreCreateBinary();

    if (attr.initial_count > 0) {
      Give();
    }
  }

  /// @brief Semaphore deleted by ~SemaphoreBase()
  ~SemaphoreBinary() = default;

  /// @brief Move ctor.
  SemaphoreBinary(SemaphoreBinary &&other) : SemaphoreBase(std::move(other)) {}

  /// @brief Move assignment.
  SemaphoreBinary &operator=(SemaphoreBinary &&other) {
    if (this != &other) {
      this->~SemaphoreBinary();
      this->handle_ = other.handle_;
      other.handle_ = nullptr;
    }

    return *this;
  }

  /// @brief Semaphore non-copyable
  SemaphoreBinary(const SemaphoreBinary &other) = delete;
  SemaphoreBinary &operator=(const SemaphoreBinary &other) = delete;
};
}  // namespace paraos

#endif /* PARAOS_SEMAPHORE_HPP */
