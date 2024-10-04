/// @file test_paraos_utils.cpp
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

#include "paraos_utils.hpp"

using namespace paraos;

TEST(MsToTimeSpec, ZeroMs) {
  auto timespec = MillisecondsInTimeSpec(0);

  ASSERT_EQ(0, timespec.tv_sec);
  ASSERT_EQ(0, timespec.tv_nsec);
}

TEST(MsToTimeSpec, Ms500) {
  auto timespec = MillisecondsInTimeSpec(500);

  ASSERT_EQ(0, timespec.tv_sec);
  ASSERT_EQ(500 * 1000 * 1000, timespec.tv_nsec);
}

TEST(MsToTimeSpec, Ms1500) {
  auto timespec = MillisecondsInTimeSpec(1500);

  ASSERT_EQ(1, timespec.tv_sec);
  ASSERT_EQ(500 * 1000 * 1000, timespec.tv_nsec);
}

TEST(MsToTimeSpec, Ms1400) {
  auto timespec = MillisecondsInTimeSpec(1400);

  ASSERT_EQ(1, timespec.tv_sec);
  ASSERT_EQ(400 * 1000 * 1000, timespec.tv_nsec);
}

TEST(MsToTimeSpec, Ms1234) {
  auto timespec = MillisecondsInTimeSpec(1234);

  ASSERT_EQ(1, timespec.tv_sec);
  ASSERT_EQ(234 * 1000 * 1000, timespec.tv_nsec);
}

TEST(MsToTimeSpec, Ms10_987) {
  auto timespec = MillisecondsInTimeSpec(10987);

  ASSERT_EQ(10, timespec.tv_sec);
  ASSERT_EQ(987 * 1000 * 1000, timespec.tv_nsec);
}
