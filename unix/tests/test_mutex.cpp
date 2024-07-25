#include <gtest/gtest.h>

#include "paraos_mutex.hpp"
#include "rtos_impl_mutex.hpp"

using namespace paraos;

TEST(Mutex, Create) {
  MutexBase mutex{MutexAttr{false}};
  ASSERT_TRUE(mutex);
}

TEST(Mutex, CreateThenUseRAII) {
  MutexBase mutex{MutexAttr{false}};
  ASSERT_TRUE(mutex);

  { const MutexGuard mutex_guard{mutex}; }

  { const MutexGuard mutex_guard{mutex}; }
}
