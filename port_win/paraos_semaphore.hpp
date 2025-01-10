/// @file paraos_semaphore.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
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

#include <stdio.h>

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_isr.hpp"
#include "paraos_utils.hpp"

namespace paraos {

struct SemaphoreAttr {
  std::size_t max_count{1u};
  std::size_t initial_count{0u};
};

class SemaphoreBase {
 public:
  operator bool() const { return handle_ != nullptr ? true : false; }

  ISRbool Take(
      std::size_t timeout_ms = max_delay, bool from_isr = false) noexcept {
    PARAOS_CHECK_ASSERT(handle_);
    PARAOS_ATTR_UNUSED_VAR(from_isr);

    // PARAOS wrapper for winapi not provided isr operations.
    PARAOS_CHECK_ASSERT(from_isr == false);

    bool is_sem_taken{false};
    if (WaitForSingleObject(handle_, static_cast<DWORD>(timeout_ms)) ==
        WAIT_OBJECT_0) {
      is_sem_taken = true;
    }
    return is_sem_taken;
  }

  /// @brief
  /// @note
  /// https://learn.microsoft.com/ru-ru/windows/win32/api/synchapi/nf-synchapi-releasesemaphore
  /// @return
  ISRbool Give(bool from_isr = false) noexcept {
    PARAOS_ATTR_UNUSED_VAR(from_isr);

    PARAOS_CHECK_ASSERT(handle_);
    constexpr LONG increment_sem_cnt{1u};

    return ISRbool{static_cast<bool>(
        ReleaseSemaphore(handle_, increment_sem_cnt, nullptr))};
  }

 protected:
  SemaphoreBase() noexcept {
#ifdef paraosTRACE_ENABLE
    std::cout << "Semaphore Ctor" << std::endl;
#endif
  }

  virtual ~SemaphoreBase() {
    if (handle_) {
      CloseHandle(handle_);

      // need for debug only
      handle_ = nullptr;
    }

#ifdef paraosTRACE_ENABLE
    std::cout << "Semaphore Dtor" << std::endl;
#endif
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

  HANDLE handle_{nullptr};
};

struct SemaphoreCounting final : public SemaphoreBase {
  SemaphoreCounting(const SemaphoreAttr &attr) : SemaphoreBase{} {
    handle_ =
        CreateSemaphore(nullptr, attr.initial_count, attr.max_count, nullptr);
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

/// @brief Класс-реализация бинарного семафора.
struct SemaphoreBinary final : public SemaphoreBase {
  /// @brief Конструктор по умолчанию для бинарного семафора.
  SemaphoreBinary() noexcept : SemaphoreBinary{SemaphoreAttr{}} {}

  SemaphoreBinary(const SemaphoreAttr &attr) noexcept : SemaphoreBase{} {
    PARAOS_ATTR_UNUSED_VAR(attr);
    constexpr LONG max_counter{1};
    constexpr LONG initial_count{0};
    handle_ = CreateSemaphore(nullptr, initial_count, max_counter, nullptr);
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
