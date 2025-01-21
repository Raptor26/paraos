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

#include <utility>

#include "paraos_semaphore.hpp"

TEST(Semaphore, CreateDefault) {
  const paraos::SemaphoreAttr attr;
  auto sem_with_def_attr = paraos::SemaphoreCounting(attr);
  ASSERT_TRUE(sem_with_def_attr);
}

TEST(Semaphore, GiveThanTakeTwice) {
  paraos::SemaphoreAttr attr;
  attr.max_count = 2U;
  auto sem = paraos::SemaphoreCounting(attr);
  ASSERT_TRUE(sem);

  ASSERT_TRUE(sem.Give());
  ASSERT_TRUE(sem.Take(0));
  ASSERT_FALSE(sem.Take(0));
}

TEST(Semaphore, GiveThanTakeTwiceButSemIsBinary) {
  auto sem = paraos::SemaphoreBinary();
  ASSERT_TRUE(sem);

  ASSERT_TRUE(sem.Give());
  ASSERT_FALSE(sem.Give());
  ASSERT_TRUE(sem.Take(0));
  ASSERT_FALSE(sem.Take(0));
}

TEST(SemaphoreBinary, MoveCtor) {
  auto sem_one = paraos::SemaphoreBinary();
  auto sem_two = paraos::SemaphoreBinary(std::move(sem_one));
}

TEST(SemaphoreBinary, MoveAssignment) {
  auto sem_one = paraos::SemaphoreBinary();
  auto sem_two = paraos::SemaphoreBinary();

  sem_one = std::move(sem_two);
}

TEST(Semaphore, BinarySemaphoreCreateNotGivenState) {
  auto sem = paraos::SemaphoreBinary();
  ASSERT_TRUE(sem);
  ASSERT_FALSE(sem.Take(0));
}

TEST(SemaphoreCounting, MoveCtor) {
  paraos::SemaphoreAttr attr;
  attr.max_count = 2U;
  auto sem_one = paraos::SemaphoreCounting(attr);
  auto sem_two = paraos::SemaphoreCounting(std::move(sem_one));
}

TEST(SemaphoreCounting, MoveAssignment) {
  paraos::SemaphoreAttr attr;
  attr.max_count = 2U;
  auto sem_one = paraos::SemaphoreCounting(attr);
  auto sem_two = paraos::SemaphoreCounting(attr);

  sem_one = std::move(sem_two);
}