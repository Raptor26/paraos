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

TEST(Mutex, CreateBinaryThenLockAndUlockTwice) {
  auto mutex_binary = MutexBaseBinary();

  ASSERT_TRUE(mutex_binary.Lock(0));
  ASSERT_FALSE(mutex_binary.Lock(0));
  ASSERT_TRUE(mutex_binary.Unlock());
  ASSERT_FALSE(mutex_binary.Unlock());
}

TEST(Mutex, LockThenUnlockTwice) {
  // В реализации RTOS вызов метода Lock() более одного раза без вызова метода
  // Unlock() возможен только в случае с рекурсивными мьютексами.
  auto default_ctor = RecursiveMutex();
  ASSERT_TRUE(default_ctor);

  ASSERT_TRUE(default_ctor.Lock(0));
  ASSERT_TRUE(default_ctor.Lock(5000));
  ASSERT_TRUE(default_ctor.Unlock());
  ASSERT_TRUE(default_ctor.Unlock());
}

TEST(Mutex, LockThenUnlockThreeTimes) {
  // В реализации RTOS вызов метода Lock() более одного раза без вызова метода
  // Unlock() возможен только в случае с рекурсивными мьютексами.
  auto default_ctor = RecursiveMutex();
  ASSERT_TRUE(default_ctor);

  ASSERT_TRUE(default_ctor.Lock(0));
  ASSERT_TRUE(default_ctor.Lock(5000));
  ASSERT_TRUE(default_ctor.Lock(200));
  ASSERT_TRUE(default_ctor.Unlock());
  ASSERT_TRUE(default_ctor.Unlock());
  ASSERT_TRUE(default_ctor.Unlock());
}

TEST(Mutex, LockThenUnlockWithRAII) {
  auto default_ctor = MutexBase();
  ASSERT_TRUE(default_ctor);

  { auto mutex_raii = MutexGuard(default_ctor); }

  ASSERT_TRUE(default_ctor);
}
