/// @file test_paraos_utils.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <gtest/gtest.h>

#include "paraos_utils.hpp"

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEST(MsToTimeSpec, ZeroMs) {
  auto timespec = paraos::MillisecondsInTimeSpec(0);

  ASSERT_EQ(0, timespec.tv_sec);
  ASSERT_EQ(0, timespec.tv_nsec);
}

TEST(MsToTimeSpec, Ms500) {
  auto timespec = paraos::MillisecondsInTimeSpec(500);

  ASSERT_EQ(0, timespec.tv_sec);
  ASSERT_EQ(500 * 1000 * 1000, timespec.tv_nsec);
}

TEST(MsToTimeSpec, Ms1500) {
  auto timespec = paraos::MillisecondsInTimeSpec(1500);

  ASSERT_EQ(1, timespec.tv_sec);
  ASSERT_EQ(500 * 1000 * 1000, timespec.tv_nsec);
}

TEST(MsToTimeSpec, Ms1400) {
  auto timespec = paraos::MillisecondsInTimeSpec(1400);

  ASSERT_EQ(1, timespec.tv_sec);
  ASSERT_EQ(400 * 1000 * 1000, timespec.tv_nsec);
}

TEST(MsToTimeSpec, Ms1234) {
  auto timespec = paraos::MillisecondsInTimeSpec(1234);

  ASSERT_EQ(1, timespec.tv_sec);
  ASSERT_EQ(234 * 1000 * 1000, timespec.tv_nsec);
}

TEST(MsToTimeSpec, Ms10_987) {
  auto timespec = paraos::MillisecondsInTimeSpec(10987);

  ASSERT_EQ(10, timespec.tv_sec);
  ASSERT_EQ(987 * 1000 * 1000, timespec.tv_nsec);
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)