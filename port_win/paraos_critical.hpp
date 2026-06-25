/// @file paraos_critical.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_CRITICAL_HPP
#define PARAOS_CRITICAL_HPP

#include <synchapi.h>

#include <cassert>

#include "paraos_attr.h"
#include "paraos_config.hpp"

namespace paraos {

class critical_section_factory final {
 public:
  /// @brief Construct a new critical section factory object.
  /// @see
  /// https://learn.microsoft.com/en-us/windows/win32/sync/using-critical-section-objects
  ///
  critical_section_factory() noexcept {
    // NOLINTBEGIN(*-magic-numbers)
    auto status =
        InitializeCriticalSectionAndSpinCount(&critical_section_, 0x00000400);
    // NOLINTEND(*-magic-numbers)

    assert(status != 0);
    PARAOS_ATTR_UNUSED_VAR(status);
  }

  ~critical_section_factory() { DeleteCriticalSection(&critical_section_); }

  /// @brief Five rule.
  critical_section_factory(critical_section_factory&& other) = delete;
  auto operator=(critical_section_factory&& other)
      -> critical_section_factory& = delete;
  auto operator=(const critical_section_factory& other)
      -> critical_section_factory& = delete;
  critical_section_factory(const critical_section_factory& other) = delete;

  auto handle() { return &critical_section_; }

  PARAOS_DEPRECATED("use handle()")
  auto GiveHandle() { return handle(); }

 private:
  CRITICAL_SECTION critical_section_{};
};

using CriticalSectionFactory PARAOS_DEPRECATED(
    "use paraos::critical_section_factory") = critical_section_factory;

template <bool CAN_ISR = true>
class critical_section final {
 public:
  /// @brief Constructor ensures automatic critical section entry.
  ///
  /// @param is_isr
  explicit critical_section(bool is_isr = false) noexcept {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    EnterCriticalSection(instance());
  }

  /// @brief Destructor ensures automatic leaving of the critical section.
  ~critical_section() { LeaveCriticalSection(instance()); }

  /// @brief Method is used for force disabling ISRs.
  ///
  /// @param[in] is_isr: This param here is only used for methods template sync.
  ///
  /// @note This method is used inside ETL libray macros.
  static void force_enter(bool is_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    EnterCriticalSection(instance());
  }

  /// @brief Method is used for force enabling ISRs.
  ///
  /// @param[in] is_isr: This param here is only used for methods template sync.
  ///
  /// @note This method is used inside ETL libray macros.
  static void force_exit(bool is_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    LeaveCriticalSection(instance());
  }

  PARAOS_DEPRECATED("use force_enter()")
  static void ForceEnter(bool is_isr = false) {
    force_enter(is_isr);
  }

  PARAOS_DEPRECATED("use force_exit()")
  static void ForceExit(bool is_isr = false) {
    force_exit(is_isr);
  }

  /// @brief Five rule.
  critical_section(critical_section&& other) = delete;
  auto operator=(critical_section&& other) -> critical_section& = delete;
  auto operator=(const critical_section& other) -> critical_section& = delete;
  critical_section(const critical_section& other) = delete;

 private:
  static auto instance() -> LPCRITICAL_SECTION {
    static critical_section_factory factory;
    return factory.handle();
  }
};

template <bool CAN_ISR = true>
using CriticalSection PARAOS_DEPRECATED("use paraos::critical_section") =
    critical_section<CAN_ISR>;

inline void disable_isr() { critical_section<true>::force_enter(); }
inline void enable_isr() { critical_section<true>::force_exit(); }

PARAOS_DEPRECATED("use disable_isr()") inline void DisableIsr() {
  disable_isr();
}
PARAOS_DEPRECATED("use enable_isr()") inline void EnableIsr() {
  enable_isr();
}

}  // namespace paraos

#endif /* PARAOS_CRITICAL_HPP */
