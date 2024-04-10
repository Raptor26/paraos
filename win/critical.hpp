/// @file critical.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @brief
///
/// @version 0.1.0
/// @date 2024-04-09
///
/// @copyright Copyright (c) 2024 Mickle Isaev
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

#ifndef CRITICAL_HPP
#define CRITICAL_HPP

#include <Windows.h>
#include <assert.h>

#include <iostream>

namespace paraos {

class CriticalSectionFactory final {
 public:
  CriticalSectionFactory() noexcept {
    std::cout << "CriticalSectionFactory Ctor" << std::endl;
    if (!InitializeCriticalSectionAndSpinCount(&critical_section_, 0x00000400))
      assert(true == false);
  }

  ~CriticalSectionFactory() {
    std::cout << "CriticalSectionFactory Dtor" << std::endl;
    DeleteCriticalSection(&critical_section_);
  }

  LPCRITICAL_SECTION GiveHandle() { return &critical_section_; }

 private:
  CRITICAL_SECTION critical_section_;
};

class CriticalSection final {
 public:
  /// @brief Конструктор обеспечивает автоматический вход в критическую секцию.
  /// @param is_isr
  CriticalSection(bool is_isr = false) : is_isr_{is_isr} {
    EnterCriticalSection(critical_section_factory.GiveHandle());
  }

  /// @brief Деструктор обеспечивает автоматический выход из критической секции.
  ~CriticalSection() {
    LeaveCriticalSection(critical_section_factory.GiveHandle());
  }

 private:
  const bool is_isr_;
  static inline CriticalSectionFactory critical_section_factory;
};

}  // namespace paraos

#endif /* CRITICAL_HPP */
