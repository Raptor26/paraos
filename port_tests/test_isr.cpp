/// @file test_isr.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <gtest/gtest.h>

#include <utility>

#include "paraos_isr.hpp"

TEST(Isr, DefaultCtor) {
  const paraos::isr_bool isr_bool;

  ASSERT_FALSE(static_cast<bool>(isr_bool));
  ASSERT_FALSE(isr_bool.needs_context_switch());
}

TEST(Isr, CtorWithTrueResult) {
  const paraos::isr_bool isr_bool{true};

  ASSERT_TRUE(static_cast<bool>(isr_bool));
  ASSERT_FALSE(isr_bool.needs_context_switch());
}

TEST(Isr, CtorWithTrueResultAndNeedSwitchContext) {
  const paraos::isr_bool isr_bool{true, true};

  ASSERT_TRUE(static_cast<bool>(isr_bool));
  ASSERT_TRUE(isr_bool.needs_context_switch());
}

TEST(Isr, CopyCtorIfFalseResultAndNoSwitchContext) {
  const paraos::isr_bool isr_bool;

  // Code below need to test copy ctor
  // NOLINTNEXTLINE(*-unnecessary-copy-initialization)
  const auto isr_bool_move{isr_bool};

  ASSERT_FALSE(static_cast<bool>(isr_bool_move));
  ASSERT_FALSE(isr_bool_move.needs_context_switch());
}

TEST(Isr, MoveCtorIfFalseResultAndNoSwitchContext) {
  paraos::isr_bool isr_bool;

  // Code below need to test move ctor
  // NOLINTNEXTLINE(*-unnecessary-copy-initialization)
  const auto isr_bool_move{std::move(isr_bool)};

  ASSERT_FALSE(static_cast<bool>(isr_bool_move));
  ASSERT_FALSE(isr_bool_move.needs_context_switch());
}

TEST(Isr, CopyAssignmentIfFalseResultAndNoSwitchContext) {
  const paraos::isr_bool isr_bool;

  // Code below need to test copy operator.
  // NOLINTNEXTLINE(*-unnecessary-copy-initialization)
  const auto isr_bool_copy = isr_bool;

  ASSERT_FALSE(static_cast<bool>(isr_bool_copy));
  ASSERT_FALSE(isr_bool_copy.needs_context_switch());
}

TEST(Isr, CopyAssignmentIfFalseResultAndNeedSwitchContext) {
  const paraos::isr_bool isr_bool{false, true};
  paraos::isr_bool isr_bool_copy{false, false};

  isr_bool_copy = isr_bool;

  ASSERT_FALSE(static_cast<bool>(isr_bool_copy));
  ASSERT_TRUE(isr_bool_copy.needs_context_switch());
}

TEST(Isr, CopyAssignmentIfFalseResultAndNeedSwitchContextInCopy) {
  const paraos::isr_bool isr_bool{false, false};
  paraos::isr_bool isr_bool_copy{false, true};

  isr_bool_copy = isr_bool;

  ASSERT_FALSE(static_cast<bool>(isr_bool_copy));
  ASSERT_TRUE(isr_bool_copy.needs_context_switch());
}

TEST(Isr, MoveAssignmentIfFalseResultAndNoSwitchContext) {
  paraos::isr_bool isr_bool;

  const auto isr_bool_copy = std::move(isr_bool);

  ASSERT_FALSE(static_cast<bool>(isr_bool_copy));
  ASSERT_FALSE(isr_bool_copy.needs_context_switch());
}
