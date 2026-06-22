/// @file paraos_critical.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef CRITICAL_HPP
#define CRITICAL_HPP

#include "paraos_mutex.hpp"

namespace paraos {

template <bool CAN_ISR = true>
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

inline void DisableIsr() { CriticalSection<false>::ForceEnter(); }
inline void EnableIsr() { CriticalSection<false>::ForceExit(); }

}  // namespace paraos

#endif /* CRITICAL_HPP */
