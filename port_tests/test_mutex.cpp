/// @file test_mutex.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <gtest/gtest.h>

#include <utility>

#include "paraos_mutex.hpp"
#include "paraos_mutex_raii.hpp"

TEST(Mutex, Create) {
  auto default_ctor = paraos::Mutex();
  ASSERT_TRUE(default_ctor);
}

TEST(Mutex, LockTwice) {
  auto default_ctor = paraos::Mutex();

  ASSERT_TRUE(default_ctor.Lock(0));

  // Can't lock twice if non recursive mutex.
  ASSERT_FALSE(default_ctor.Lock(0));
}

TEST(Mutex, LockThenUnlock) {
  auto default_ctor = paraos::Mutex();

  ASSERT_TRUE(default_ctor.Lock(0));
  ASSERT_TRUE(default_ctor.Unlock());

  // Can't unlock twice if non recursive mutex.
  ASSERT_FALSE(default_ctor.Unlock());
}

TEST(Mutex, LockThenUnlockWithRAII) {
  auto default_ctor = paraos::Mutex();

  {
    auto mutex_raii = paraos::MutexGuard(default_ctor);
  }
}

TEST(Mutex, MoveCtor) {
  auto mutex = paraos::Mutex();
  auto move_to_me = std::move(mutex);
}

TEST(Mutex, MoveAssignment) {
  auto mutex_one = paraos::Mutex();
  auto mutex_two = paraos::Mutex();
  mutex_two = std::move(mutex_one);
}

TEST(MutexRecursive, LockThenUnlockTwice) {
  auto default_ctor = paraos::MutexRecursive();
  ASSERT_TRUE(default_ctor);

  ASSERT_TRUE(default_ctor.Lock(0));
  ASSERT_TRUE(default_ctor.Lock(5000));
  ASSERT_TRUE(default_ctor.Unlock());
  ASSERT_TRUE(default_ctor.Unlock());

  // Mutex lock twice, and unlock twice too, next release not succeed.
  ASSERT_FALSE(default_ctor.Unlock());
}

TEST(MutexRecursive, MoveCtor) {
  auto mutex = paraos::MutexRecursive();
  auto move_to_me = std::move(mutex);
}

TEST(MutexRecursive, MoveAssignment) {
  auto mutex_one = paraos::MutexRecursive();
  auto mutex_two = paraos::MutexRecursive();
  mutex_two = std::move(mutex_one);
}