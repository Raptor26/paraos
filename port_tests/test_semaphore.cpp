/// @file test_semaphore.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
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