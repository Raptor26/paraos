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

#ifndef CRITICAL_HPP
#define CRITICAL_HPP

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

#include "paraos_mutex.hpp"

namespace paraos {

class CriticalSection final {
 public:
  /// @brief Constructor ensures automatic critical section entry.
  ///
  /// @param is_isr
  explicit CriticalSection(bool is_isr = false) : is_isr_{is_isr} {
    mutex_.Lock(max_delay, is_isr_);
  }

  /// @brief Destructor ensures automatic leaving of the critical section.
  ~CriticalSection() { mutex_.Unlock(is_isr_); }

  /// @brief Method is used for force disabling ISRs.
  ///
  /// @param[in] is_isr: This param here is only used for methods template sync.
  ///
  /// @note This method is used inside ETL libray macros.
  static void ForceEnter(bool is_isr = false) {
    mutex_.Lock(max_delay, is_isr);
  }

  /// @brief Method is used for force enabling ISRs.
  ///
  /// @param[in] is_isr: This param here is only used for methods template sync.
  ///
  /// @note This method is used inside ETL libray macros.
  static void ForceExit(bool is_isr = false) { mutex_.Unlock(is_isr); }

  /// @brief Five rule.
  CriticalSection(CriticalSection &&other) = delete;
  auto operator=(CriticalSection &&other) -> CriticalSection & = delete;
  auto operator=(const CriticalSection &other) -> CriticalSection & = delete;
  CriticalSection(const CriticalSection &other) = delete;

 private:
  const bool is_isr_;
  static inline MutexRecursive mutex_;
};

inline void DisableIsr() { CriticalSection::ForceEnter(); }
inline void EnableIsr() { CriticalSection::ForceExit(); }

}  // namespace paraos

#endif /* CRITICAL_HPP */
