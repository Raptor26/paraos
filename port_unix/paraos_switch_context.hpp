/// @file paraos_switch_context.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_SWITCH_CONTEXT_HPP
#define PARAOS_SWITCH_CONTEXT_HPP

#include "paraos_attr.h"
#include "paraos_isr.hpp"

namespace paraos {

/// @brief Automaticaly switch thread context if need. Use WritePrimitiveState()
/// for check is need switch thread context.
///
/// @note paraos not support execute from isr in unix. In this case, unix
/// port use mock for SwitchContext().
class SwitchContext final {
 public:
  SwitchContext() = default;

  /// NOLINTNEXTLINE(*-convert-member-functions-to-static)
  void WritePrimitiveState(const isr_bool &primitive_state) {
    PARAOS_ATTR_UNUSED_VAR(primitive_state);
  }

  ~SwitchContext() = default;

  /// @brief Five rule.
  SwitchContext(SwitchContext &&other) = delete;
  auto operator=(SwitchContext &&other) -> SwitchContext & = delete;
  auto operator=(const SwitchContext &other) -> SwitchContext & = delete;
  SwitchContext(const SwitchContext &other) = delete;
};

}  // namespace paraos

#endif /* PARAOS_SWITCH_CONTEXT_HPP */
