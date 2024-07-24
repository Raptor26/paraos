#include <gtest/gtest.h>

#include "paraos_mutex.hpp"
#include "rtos_impl_mutex.hpp"

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
  ASSERT_TRUE(default_ctor.Lock(0));
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
