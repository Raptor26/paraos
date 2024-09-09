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

#include "paraos_mutex.hpp"
#include "paraos_mutex_raii.hpp"

using namespace paraos;

TEST(Mutex, Create) {
  MutexAttr attr;
  auto parametrize_ctor = MutexBase(attr);
  ASSERT_TRUE(parametrize_ctor);

  auto default_ctor = MutexBase();
  ASSERT_TRUE(default_ctor);
}

TEST(Mutex, LockThenUnlock) {
  auto default_ctor = MutexBase();
  ASSERT_TRUE(default_ctor);

  ASSERT_TRUE(default_ctor.Lock(0));
  ASSERT_TRUE(default_ctor.Unlock());
}

TEST(Mutex, LockThenUnlockTwice) {
  auto default_ctor = MutexBase();
  ASSERT_TRUE(default_ctor);

  ASSERT_TRUE(default_ctor.Lock(0));
  ASSERT_TRUE(default_ctor.Lock(5000));
  ASSERT_TRUE(default_ctor.Unlock());
  ASSERT_TRUE(default_ctor.Unlock());
}

TEST(Mutex, LockThenUnlockWithRAII) {
  auto default_ctor = MutexBase();
  ASSERT_TRUE(default_ctor);

  { auto mutex_raii = MutexGuard(default_ctor); }

  ASSERT_TRUE(default_ctor);
}

TEST(Mutex, CreateBinaryThenLockAndUlockTwice) {
  auto mutex_binary = MutexBaseBinary();

  ASSERT_TRUE(mutex_binary.Lock(0));
  ASSERT_FALSE(mutex_binary.Lock(0));
  ASSERT_TRUE(mutex_binary.Unlock());
  ASSERT_FALSE(mutex_binary.Unlock());
}
