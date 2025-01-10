/// @file test_semaphore.cpp
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

#include "paraos_semaphore.hpp"

using namespace paraos;

TEST(Semaphore, CreateDefault) {
  SemaphoreAttr attr;
  auto sem_with_def_attr = SemaphoreCounting(attr);
  ASSERT_TRUE(sem_with_def_attr);
}

TEST(Semaphore, GiveThanTakeTwice) {
  SemaphoreAttr attr;
  attr.max_count = 2u;
  auto sem = SemaphoreCounting(attr);
  ASSERT_TRUE(sem);

  ASSERT_TRUE(sem.Give());
  ASSERT_TRUE(sem.Take(0));
  ASSERT_FALSE(sem.Take(0));
}

TEST(Semaphore, GiveThanTakeTwiceButSemIsBinary) {
  auto sem = SemaphoreBinary();
  ASSERT_TRUE(sem);

  ASSERT_TRUE(sem.Give());
  ASSERT_FALSE(sem.Give());
  ASSERT_TRUE(sem.Take(0));
  ASSERT_FALSE(sem.Take(0));
}

TEST(SemaphoreBinary, MoveCtor) {
  auto sem_one = SemaphoreBinary();
  auto sem_two = SemaphoreBinary(std::move(sem_one));
}

TEST(SemaphoreBinary, MoveAssignment) {
  auto sem_one = SemaphoreBinary();
  auto sem_two = SemaphoreBinary();

  sem_one = std::move(sem_two);
}

TEST(Semaphore, BinarySemaphoreCreateNotGivenState) {
  auto sem = SemaphoreBinary();
  ASSERT_TRUE(sem);
  ASSERT_FALSE(sem.Take(0));
}

TEST(SemaphoreCounting, MoveCtor) {
  SemaphoreAttr attr;
  attr.max_count = 2u;
  auto sem_one = SemaphoreCounting(attr);
  auto sem_two = SemaphoreCounting(std::move(sem_one));
}

TEST(SemaphoreCounting, MoveAssignment) {
  SemaphoreAttr attr;
  attr.max_count = 2u;
  auto sem_one = SemaphoreCounting(attr);
  auto sem_two = SemaphoreCounting(attr);

  sem_one = std::move(sem_two);
}