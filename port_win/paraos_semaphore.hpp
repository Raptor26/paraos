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

#include <utility>

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_isr.hpp"
#include "paraos_utils.hpp"

namespace paraos {

struct SemaphoreAttr {
  std::size_t max_count{1U};
  std::size_t initial_count{0U};
};

class SemaphoreBase {
 public:
  explicit operator bool() const {
    return static_cast<bool>(handle_ != nullptr);
  }

  auto Take(std::size_t timeout_ms = max_delay, bool from_isr = false) noexcept
      -> ISRbool {
    PARAOS_CHECK_ASSERT(handle_);
    PARAOS_ATTR_UNUSED_VAR(from_isr);

    // PARAOS wrapper for winapi not provided isr operations.
    PARAOS_CHECK_ASSERT(from_isr == false);

    bool is_sem_taken{false};
    if (WaitForSingleObject(handle_, static_cast<DWORD>(timeout_ms)) ==
        WAIT_OBJECT_0) {
      is_sem_taken = true;
    }
    return static_cast<ISRbool>(is_sem_taken);
  }

  /// @brief
  /// @note
  /// https://learn.microsoft.com/ru-ru/windows/win32/api/synchapi/nf-synchapi-releasesemaphore
  /// @return
  auto Give(bool from_isr = false) noexcept -> ISRbool {
    PARAOS_ATTR_UNUSED_VAR(from_isr);

    PARAOS_CHECK_ASSERT(handle_);
    constexpr LONG increment_sem_cnt{1U};

    return ISRbool{static_cast<bool>(
        ReleaseSemaphore(handle_, increment_sem_cnt, nullptr))};
  }

  /// @brief Semaphore non-copyable
  SemaphoreBase(const SemaphoreBase &other) = delete;
  auto operator=(const SemaphoreBase &other) -> SemaphoreBase & = delete;

 protected:
  SemaphoreBase() noexcept {
#ifdef paraosTRACE_ENABLE
    std::cout << "Semaphore Ctor" << std::endl;
#endif
  }

  virtual ~SemaphoreBase() {
    if (handle_ != nullptr) {
      CloseHandle(handle_);

      // need for debug only
      handle_ = nullptr;
    }

#ifdef paraosTRACE_ENABLE
    std::cout << "Semaphore Dtor" << std::endl;
#endif
  }

  /// @brief Move ctor.
  SemaphoreBase(SemaphoreBase &&other) noexcept {
    if (this != &other) {
      this->handle_ = other.handle_;
      other.handle_ = nullptr;
    }
  }

  /// @brief Move assignment.
  auto operator=(SemaphoreBase &&other) noexcept -> SemaphoreBase & {
    if (this != &other) {
      this->~SemaphoreBase();
      this->handle_ = other.handle_;
      other.handle_ = nullptr;
    }

    return *this;
  }

  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  // We can't put this variable into private section, because it's used in
  // derived classes.
  HANDLE handle_{nullptr};
  // NOLINTEND(misc-non-private-member-variables-in-classes)
};

struct SemaphoreCounting final : public SemaphoreBase {
  explicit SemaphoreCounting(const SemaphoreAttr &attr) {
    handle_ =
        CreateSemaphore(nullptr, attr.initial_count, attr.max_count, nullptr);
  }

  /// @brief Semaphore deleted by ~SemaphoreBase()
  ~SemaphoreCounting() override = default;

  /// @brief Move ctor.
  SemaphoreCounting(SemaphoreCounting &&other) noexcept
      : SemaphoreBase(std::move(other)) {}

  /// @brief Move assignment.
  auto operator=(SemaphoreCounting &&other) noexcept -> SemaphoreCounting & {
    if (this != &other) {
      this->~SemaphoreCounting();
      this->handle_ = other.handle_;
      other.handle_ = nullptr;
    }

    return *this;
  }

  /// @brief Semaphore non-copyable
  SemaphoreCounting(const SemaphoreCounting &other) = delete;
  auto operator=(const SemaphoreCounting &other)
      -> SemaphoreCounting & = delete;
};

/// @brief Класс-реализация бинарного семафора.
struct SemaphoreBinary final : public SemaphoreBase {
  /// @brief Конструктор по умолчанию для бинарного семафора.
  SemaphoreBinary() noexcept : SemaphoreBinary{SemaphoreAttr{}} {}

  explicit SemaphoreBinary(const SemaphoreAttr &attr) noexcept {
    PARAOS_ATTR_UNUSED_VAR(attr);
    constexpr LONG max_counter{1};
    constexpr LONG initial_count{0};
    handle_ = CreateSemaphore(nullptr, initial_count, max_counter, nullptr);
  }

  /// @brief Semaphore deleted by ~SemaphoreBase()
  ~SemaphoreBinary() override = default;

  /// @brief Move ctor.
  SemaphoreBinary(SemaphoreBinary &&other) noexcept
      : SemaphoreBase(std::move(other)) {}

  /// @brief Move assignment.
  auto operator=(SemaphoreBinary &&other) noexcept -> SemaphoreBinary & {
    if (this != &other) {
      this->~SemaphoreBinary();
      this->handle_ = other.handle_;
      other.handle_ = nullptr;
    }

    return *this;
  }

  /// @brief Semaphore non-copyable
  SemaphoreBinary(const SemaphoreBinary &other) = delete;
  auto operator=(const SemaphoreBinary &other) -> SemaphoreBinary & = delete;
};

}  // namespace paraos

#endif /* PARAOS_SEMAPHORE_HPP */
