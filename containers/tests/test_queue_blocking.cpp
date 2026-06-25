/// @file test_queue_blocking.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <gtest/gtest.h>

#include <cstddef>
#include <mutex>

#include "paraos_jthread.hpp"
#include "paraos_mutex_std.hpp"
#include "paraos_queue_blocking.hpp"
#include "paraos_semaphore_std.hpp"

constexpr std::size_t block_time_ms{0};

TEST(QueueBlocking, Create) { const paraos::queue_blocking<int, 20> queue; }

TEST(QueueBlocking, EmplaceThenRead) {
  constexpr std::size_t max_elem{3};
  paraos::queue_blocking<int, max_elem> queue;

  ASSERT_TRUE(queue.try_push(1));
  ASSERT_TRUE(queue.try_push(2));
  ASSERT_TRUE(queue.try_push(3));
  ASSERT_FALSE(queue.try_push(4));

  {
    auto result = queue.pop(block_time_ms);
    ASSERT_TRUE(result);
    ASSERT_EQ(1, *result);
  }

  {
    auto result = queue.pop(block_time_ms);
    ASSERT_TRUE(result);
    ASSERT_EQ(2, *result);
  }

  {
    auto result = queue.pop(block_time_ms);
    ASSERT_TRUE(result);
    ASSERT_EQ(3, *result);
  }

  {
    auto result = queue.pop(block_time_ms);
    ASSERT_FALSE(result);
  }
}

TEST(QueueBlocking, TryPushThenRead) {
  constexpr std::size_t max_elem{2};
  paraos::queue_blocking<int, max_elem> queue;

  constexpr int val{1};
  ASSERT_TRUE(queue.try_push(val));     // TryPush lvalue
  ASSERT_TRUE(queue.try_push(int{2}));  // TryPush rvalue
  ASSERT_FALSE(queue.try_push(3));

  {
    auto result = queue.pop(block_time_ms);
    ASSERT_TRUE(result);
    ASSERT_EQ(1, *result);
  }

  {
    auto result = queue.pop(block_time_ms);
    ASSERT_TRUE(result);
    ASSERT_EQ(2, *result);
  }

  {
    auto result = queue.pop(block_time_ms);
    ASSERT_FALSE(result);
  }

  ASSERT_TRUE(queue.try_push(int{7}));
}

TEST(QueueBlocking, PopOnEmptyQueue) {
  constexpr std::size_t max_elem{2};
  paraos::queue_blocking<int, max_elem> queue;

  {
    // Метод Pop() не дождётся семафора TryPush_sem (не было выполнено вставок)
    // и вернёт значение по умолчанию для указанного типа данных.
    auto result = queue.pop(block_time_ms);
    ASSERT_FALSE(result);
  }
}

TEST(MutexStd, LockGuardAndUniqueLockCompileAndRun) {
  paraos::mutex mtx;
  {
    const std::scoped_lock<paraos::mutex> lock{mtx};
    // Critical section
  }
  {
    std::unique_lock<paraos::mutex> lock{mtx};
    ASSERT_TRUE(lock.owns_lock());
    lock.unlock();
    ASSERT_FALSE(lock.owns_lock());
  }
}

TEST(SemaphoreStd, BinarySemaphoreReleaseAndAcquire) {
  paraos::binary_semaphore sem{0};
  sem.release();
  ASSERT_TRUE(sem.try_acquire());
  ASSERT_FALSE(sem.try_acquire());
}

TEST(SemaphoreStd, CountingSemaphoreReleaseNAndAcquire) {
  paraos::counting_semaphore<3> sem{0};
  sem.release(2);
  ASSERT_TRUE(sem.try_acquire());
  ASSERT_TRUE(sem.try_acquire());
  ASSERT_FALSE(sem.try_acquire());
}
