#include <gtest/gtest.h>

#include "semaphore.hpp"

using namespace paraos;

TEST(Semaphore, CreateDefault) {
  SemaphoreAttr attr;
  auto sem_with_def_attr = Semaphore(attr);
  ASSERT_TRUE(sem_with_def_attr);

  auto sem_default_ctor = Semaphore();
  ASSERT_TRUE(sem_default_ctor);
}

TEST(Semaphore, GiveThanTakeTwice) {
  SemaphoreAttr attr;
  attr.max_count = 2u;
  auto sem = Semaphore(attr);
  ASSERT_TRUE(sem);

  ASSERT_TRUE(sem.Give());
  ASSERT_TRUE(sem.Take(0));
}

TEST(Semaphore, GiveThanTakeTwiceButSemIsBinary) {
  auto sem = SemaphoreBinary();
  ASSERT_TRUE(sem);

  ASSERT_TRUE(sem.Give());
  ASSERT_FALSE(sem.Give());
  ASSERT_TRUE(sem.Take(0));
  ASSERT_FALSE(sem.Take(0));
}
