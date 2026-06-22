/// @file test_version.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <gtest/gtest.h>

#include <string_view>

#include "paraos_version.hpp"

TEST(Version, PrintVersion) {
  // Check compiler errors only.
  constexpr std::string_view version_prefix{"PARAOS version is"};

  // Expect that string started with prefix ”version_prefix”.
  EXPECT_EQ(0, paraos::version.find(version_prefix));
}