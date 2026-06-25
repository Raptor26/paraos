/// @file test_critical.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <gtest/gtest.h>

#include "paraos_critical.hpp"
#include "paraos_trace.hpp"

TEST(Critical, WithISR) {
  const paraos::critical_section critical;

  // To print message below use cmake presets with «*_trace».
  paraosOUT(sizeof(critical));
}

TEST(Critical, WithoutISR) {
  const paraos::critical_section<false> critical;

  ASSERT_EQ(sizeof(critical), 1U);
}