/// @file paraos_switch_context.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_SWITCH_CONTEXT_HPP
#define PARAOS_SWITCH_CONTEXT_HPP

#include "FreeRTOS.h"
#include "paraos_attr.h"
#include "paraos_isr.hpp"

namespace paraos {

/// @brief Automaticaly switch thread context if need. Use WritePrimitiveState()
/// for check is need switch thread context. When Dtor was calling,
/// SwitchContext check and switch context if was request in
/// WritePrimitiveState() calling.
class SwitchContext final {
 public:
  SwitchContext() = default;

  /// @brief Update is need switch context state by primitive state.
  ///
  /// @param[in] primitive_state: ISRbool return Take()/Give()  Semaphore/mutex
  /// API.
  void WritePrimitiveState(const ISRbool &primitive_state) noexcept {
    if (primitive_state.IsNeedSwitchContext()) {
      is_need_switch_context_ = true;
    }
  }

  ~SwitchContext() {
    if (is_need_switch_context_) {
      portYIELD();
    }
  }

  /// @brief Five rule.
  SwitchContext(SwitchContext &&other) = delete;
  auto operator=(SwitchContext &&other) -> SwitchContext & = delete;
  auto operator=(const SwitchContext &other) -> SwitchContext & = delete;
  SwitchContext(const SwitchContext &other) = delete;

 private:
  bool is_need_switch_context_{false};
};

}  // namespace paraos

#endif /* PARAOS_SWITCH_CONTEXT_HPP */
