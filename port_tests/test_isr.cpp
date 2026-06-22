/// @file test_isr.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <gtest/gtest.h>

#include <utility>

#include "paraos_isr.hpp"

TEST(Isr, DefaultCtor) {
  const paraos::ISRbool isr_bool;

  ASSERT_FALSE(static_cast<bool>(isr_bool));
  ASSERT_FALSE(isr_bool.IsNeedSwitchContext());
}

TEST(Isr, CtorWithTrueResult) {
  const paraos::ISRbool isr_bool{true};

  ASSERT_TRUE(static_cast<bool>(isr_bool));
  ASSERT_FALSE(isr_bool.IsNeedSwitchContext());
}

TEST(Isr, CtorWithTrueResultAndNeedSwitchContext) {
  const paraos::ISRbool isr_bool{true, true};

  ASSERT_TRUE(static_cast<bool>(isr_bool));
  ASSERT_TRUE(isr_bool.IsNeedSwitchContext());
}

TEST(Isr, CopyCtorIfFalseResultAndNoSwitchContext) {
  const paraos::ISRbool isr_bool;

  // Code below need to test copy ctor
  // NOLINTNEXTLINE(*-unnecessary-copy-initialization)
  const auto isr_bool_move{isr_bool};

  ASSERT_FALSE(static_cast<bool>(isr_bool_move));
  ASSERT_FALSE(isr_bool_move.IsNeedSwitchContext());
}

TEST(Isr, MoveCtorIfFalseResultAndNoSwitchContext) {
  paraos::ISRbool isr_bool;

  // Code below need to test move ctor
  // NOLINTNEXTLINE(*-unnecessary-copy-initialization)
  const auto isr_bool_move{std::move(isr_bool)};

  ASSERT_FALSE(static_cast<bool>(isr_bool_move));
  ASSERT_FALSE(isr_bool_move.IsNeedSwitchContext());
}

TEST(Isr, CopyAssignmentIfFalseResultAndNoSwitchContext) {
  const paraos::ISRbool isr_bool;

  // Code below need to test copy operator.
  // NOLINTNEXTLINE(*-unnecessary-copy-initialization)
  const auto isr_bool_copy = isr_bool;

  ASSERT_FALSE(static_cast<bool>(isr_bool_copy));
  ASSERT_FALSE(isr_bool_copy.IsNeedSwitchContext());
}

TEST(Isr, CopyAssignmentIfFalseResultAndNeedSwitchContext) {
  const paraos::ISRbool isr_bool{false, true};
  paraos::ISRbool isr_bool_copy{false, false};

  isr_bool_copy = isr_bool;

  ASSERT_FALSE(static_cast<bool>(isr_bool_copy));
  ASSERT_TRUE(isr_bool_copy.IsNeedSwitchContext());
}

TEST(Isr, CopyAssignmentIfFalseResultAndNeedSwitchContextInCopy) {
  const paraos::ISRbool isr_bool{false, false};
  paraos::ISRbool isr_bool_copy{false, true};

  isr_bool_copy = isr_bool;

  ASSERT_FALSE(static_cast<bool>(isr_bool_copy));
  ASSERT_TRUE(isr_bool_copy.IsNeedSwitchContext());
}

TEST(Isr, MoveAssignmentIfFalseResultAndNoSwitchContext) {
  paraos::ISRbool isr_bool;

  const auto isr_bool_copy = std::move(isr_bool);

  ASSERT_FALSE(static_cast<bool>(isr_bool_copy));
  ASSERT_FALSE(isr_bool_copy.IsNeedSwitchContext());
}