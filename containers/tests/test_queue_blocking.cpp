/// @file test_queue_blocking.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author VyhodcevEgor <vyhodcev@internet.ru>
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

#include <iostream>

#include "paraos_queue_blocking.hpp"

using namespace paraos;

constexpr std::size_t block_time_ms{0};

TEST(QueueBlocking, Create) { QueueBlocking<int, 20> queue; }

TEST(QueueBlocking, EmplaceThenRead) {
  constexpr std::size_t max_elem{3};
  QueueBlocking<int, max_elem> queue;

  ASSERT_TRUE(queue.TryPush(1));
  ASSERT_TRUE(queue.TryPush(2));
  ASSERT_TRUE(queue.TryPush(3));
  ASSERT_FALSE(queue.TryPush(4));

  {
    auto result = queue.Pop(block_time_ms);
    ASSERT_TRUE(result);
    ASSERT_EQ(1, *result);
  }

  {
    auto result = queue.Pop(block_time_ms);
    ASSERT_TRUE(result);
    ASSERT_EQ(2, *result);
  }

  {
    auto result = queue.Pop(block_time_ms);
    ASSERT_TRUE(result);
    ASSERT_EQ(3, *result);
  }

  {
    auto result = queue.Pop(block_time_ms);
    ASSERT_FALSE(result);
  }
}

TEST(QueueBlocking, TryPushThenRead) {
  constexpr std::size_t max_elem{2};
  QueueBlocking<int, max_elem> queue;

  int val{1};
  ASSERT_TRUE(queue.TryPush(val));     // TryPush lvalue
  ASSERT_TRUE(queue.TryPush(int{2}));  // TryPush rvalue
  ASSERT_FALSE(queue.TryPush(3));

  {
    auto result = queue.Pop(block_time_ms);
    ASSERT_TRUE(result);
    ASSERT_EQ(1, *result);
  }

  {
    auto result = queue.Pop(block_time_ms);
    ASSERT_TRUE(result);
    ASSERT_EQ(2, *result);
  }

  {
    auto result = queue.Pop(block_time_ms);
    ASSERT_FALSE(result);
  }

  ASSERT_TRUE(queue.TryPush(int{7}));
}

TEST(QueueBlocking, PopOnEmptyQueue) {
  constexpr std::size_t max_elem{2};
  QueueBlocking<int, max_elem> queue;

  {
    // Метод Pop() не дождётся семафора TryPush_sem (не было выполнено вставок) и
    // вернёт значение по умолчанию для указанного типа данных.
    auto result = queue.Pop(block_time_ms);
    ASSERT_FALSE(result);
  }
}
