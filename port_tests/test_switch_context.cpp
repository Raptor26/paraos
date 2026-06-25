/// @file test_switch_context.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <gtest/gtest.h>

#include "paraos_attr.h"
#include "paraos_isr.hpp"
#include "paraos_switch_context.hpp"

TEST(SwitchContext, IfNothingChange) {
  const paraos::SwitchContext switch_context;
  PARAOS_ATTR_UNUSED_VAR(switch_context);
}

TEST(SwitchContext, IfNoRequestToChangeContext) {
  paraos::SwitchContext switch_context;
  const paraos::isr_bool primitive_state_default{};
  switch_context.WritePrimitiveState(primitive_state_default);
}
