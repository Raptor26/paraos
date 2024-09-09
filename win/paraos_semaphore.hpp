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

#ifndef SEMAPHORE_HPP
#define SEMAPHORE_HPP

#include <stdio.h>

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

#include "paraos_utils.hpp"

namespace paraos {

struct SemaphoreAttr {
  std::size_t max_count = 1u;
};

constexpr std::size_t initial_count = 0u;

class Semaphore {
 public:
  Semaphore(const SemaphoreAttr attr) noexcept
      : handle_{
            CreateSemaphore(nullptr, initial_count, attr.max_count, nullptr)} {
#ifdef paraosTRACE_ENABLE
    std::cout << "Semaphore Ctor" << std::endl;
#endif
  }

  virtual ~Semaphore() {
    if (handle_) {
      CloseHandle(handle_);

      // need for debug only
      handle_ = nullptr;
    }

#ifdef paraosTRACE_ENABLE
    std::cout << "Semaphore Dtor" << std::endl;
#endif
  }

  operator bool() const { return handle_ != nullptr ? true : false; }

  bool Take(std::size_t timeout_ms = max_delay) {
    assert(handle_);
    bool is_sem_taken = false;
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
  bool Give(bool from_isr = false) {
    assert(handle_);
    constexpr LONG increment_sem_cnt{1u};

    return static_cast<bool>(
        ReleaseSemaphore(handle_, increment_sem_cnt, nullptr));
  }

 protected:
  Semaphore() : Semaphore{SemaphoreAttr{}} {}

 private:
  HANDLE handle_{nullptr};
};

struct SemaphoreBinary final : public Semaphore {
  SemaphoreBinary() noexcept : Semaphore{} {}

  ~SemaphoreBinary() = default;
};

}  // namespace paraos

#endif /* SEMAPHORE_HPP */
