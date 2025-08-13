/// @file paraos_critical.hpp
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

#ifndef PARAOS_CRITICAL_HPP
#define PARAOS_CRITICAL_HPP

#include <synchapi.h>

#include <cassert>

#include "paraos_attr.h"

namespace paraos {

class CriticalSectionFactory final {
 public:
  /// @brief Construct a new Critical Section Factory object.
  /// @see
  /// https://learn.microsoft.com/en-us/windows/win32/sync/using-critical-section-objects
  ///
  CriticalSectionFactory() noexcept {
    // NOLINTBEGIN(*-magic-numbers)
    auto status =
        InitializeCriticalSectionAndSpinCount(&critical_section_, 0x00000400);
    // NOLINTEND(*-magic-numbers)

    assert(status != 0);
    PARAOS_ATTR_UNUSED_VAR(status);
  }

  ~CriticalSectionFactory() { DeleteCriticalSection(&critical_section_); }

  /// @brief Five rule.
  CriticalSectionFactory(CriticalSectionFactory&& other) = delete;
  auto operator=(CriticalSectionFactory&& other)
      -> CriticalSectionFactory& = delete;
  auto operator=(const CriticalSectionFactory& other)
      -> CriticalSectionFactory& = delete;
  CriticalSectionFactory(const CriticalSectionFactory& other) = delete;

  auto GiveHandle() { return &critical_section_; }

 private:
  CRITICAL_SECTION critical_section_{};
};

template <bool CAN_ISR = true>
class CriticalSection final {
 public:
  /// @brief Constructor ensures automatic critical section entry.
  ///
  /// @param is_isr
  explicit CriticalSection(bool is_isr = false) noexcept {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    EnterCriticalSection(GiveInstance());
  }

  /// @brief Destructor ensures automatic leaving of the critical section.
  ~CriticalSection() { LeaveCriticalSection(GiveInstance()); }

  /// @brief Method is used for force disabling ISRs.
  ///
  /// @param[in] is_isr: This param here is only used for methods template sync.
  ///
  /// @note This method is used inside ETL libray macros.
  static void ForceEnter(bool is_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    EnterCriticalSection(GiveInstance());
  }

  /// @brief Method is used for force enabling ISRs.
  ///
  /// @param[in] is_isr: This param here is only used for methods template sync.
  ///
  /// @note This method is used inside ETL libray macros.
  static void ForceExit(bool is_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    LeaveCriticalSection(GiveInstance());
  }

  /// @brief Five rule.
  CriticalSection(CriticalSection&& other) = delete;
  auto operator=(CriticalSection&& other) -> CriticalSection& = delete;
  auto operator=(const CriticalSection& other) -> CriticalSection& = delete;
  CriticalSection(const CriticalSection& other) = delete;

 private:
  static auto GiveInstance() -> LPCRITICAL_SECTION {
    static CriticalSectionFactory critical_section_factory;
    return critical_section_factory.GiveHandle();
  }
};

inline void DisableIsr() { CriticalSection<true>::ForceEnter(); }
inline void EnableIsr() { CriticalSection<true>::ForceExit(); }

}  // namespace paraos

#endif /* PARAOS_CRITICAL_HPP */
