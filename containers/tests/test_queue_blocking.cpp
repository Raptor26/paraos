#include <gtest/gtest.h>

#include <iostream>

#include "paraos_queue_blocking.hpp"

using namespace paraos;

TEST(QueueBlocking, Create) { QueueBlocking<int> queue{10}; }

TEST(QueueBlocking, EmplaceThenRead) {
  QueueBlocking<int> queue{3};

  size_t block_time_ms{0};

  ASSERT_TRUE(queue.Push(1, block_time_ms));
  ASSERT_TRUE(queue.Push(2, block_time_ms));
  ASSERT_TRUE(queue.Push(3, block_time_ms));
  ASSERT_FALSE(queue.Push(4, block_time_ms));

  {
    auto result = queue.Pop(block_time_ms);
    ASSERT_EQ(1, result);
  }

  {
    auto result = queue.Pop(block_time_ms);
    ASSERT_EQ(2, result);
  }

  {
    auto result = queue.Pop(block_time_ms);
    ASSERT_EQ(3, result);
  }

  {
    auto result = queue.Pop(block_time_ms);
    ASSERT_NE(4, result);
  }
}

TEST(QueueBlocking, PushThenRead) {
  QueueBlocking<int> queue{2};

  size_t block_time_ms{0};
  int val{1};
  ASSERT_TRUE(queue.Push(val, block_time_ms));     // push lvalue
  ASSERT_TRUE(queue.Push(int{2}, block_time_ms));  // push rvalue
  ASSERT_FALSE(queue.Push(3, block_time_ms));

  {
    auto result = queue.Pop(block_time_ms);
    ASSERT_EQ(1, result);
  }

  {
    auto result = queue.Pop(block_time_ms);
    ASSERT_EQ(2, result);
  }

  {
    auto result = queue.Pop(block_time_ms);
    ASSERT_NE(3, result);
  }

  ASSERT_TRUE(queue.Push(int{7}, block_time_ms));
}

TEST(QueueBlocking, PopOnEmptyQueue) {
  QueueBlocking<int> queue{2};

  size_t block_time_ms{0};
  // Значение по умолчанию для выражения int{} равно 0.
  int constexpr default_int_init_value{0};

  {
    // Метод Pop() не дождётся семафора push_sem (не было выполнено вставок) и
    // вернёт значение по умолчанию для указанного типа данных.
    auto result = queue.Pop(block_time_ms);
    ASSERT_EQ(default_int_init_value, result);
  }
}
