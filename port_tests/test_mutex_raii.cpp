/// @file test_mutex_raii.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <gtest/gtest.h>

#include <utility>

#include "paraos_mutex.hpp"
#include "paraos_mutex_raii.hpp"

TEST(MutexRAII, Create) {
  paraos::Mutex mutex{};
  const paraos::MutexGuard mutex_guard(mutex);

  ASSERT_TRUE(mutex_guard.IsLocked());
}