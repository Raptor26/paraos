/// @file test_mutex.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2024 Stilsoft
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