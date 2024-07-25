#include <gtest/gtest.h>

#include "paraos_semaphore.hpp"

using namespace paraos;

TEST(Semaphore, Create) { Semaphore sem; }

TEST(Semaphore, Give) {
  Semaphore sem;
  ASSERT_FALSE(sem.Take(0u));
  ASSERT_TRUE(sem.Give());
  ASSERT_TRUE(sem.Take());
}