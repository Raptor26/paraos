/// @file test_paraos_isr.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2025 Stilsoft
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