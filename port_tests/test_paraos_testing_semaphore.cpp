/// @file test_paraos_testing_semaphore.cpp
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
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

#include "paraos_testing_semaphore.hpp"

TEST(TestingSemaphore, CreateDefault) {
  paraos::TestingSemaphoreAttr attrs{};

  paraos::TestingSemaphore semaphore{attrs};
}

TEST(TestingSemaphore, TakeWithoutGive) {
  paraos::TestingSemaphoreAttr attrs{};

  attrs.initial_count = 0;

  paraos::TestingSemaphore semaphore{attrs};

  ASSERT_FALSE(semaphore.Take());
}

TEST(TestingSemaphore, MultipleGiveAndTake) {
  paraos::TestingSemaphoreAttr attrs{};

  attrs.max_count = 3;
  attrs.initial_count = 0;

  paraos::TestingSemaphore semaphore{attrs};

  // Поскольку максимальное значение счётчика семафора равно 3, отдать семафор
  // можно не более 3 раз.
  ASSERT_TRUE(semaphore.Give());

  ASSERT_TRUE(semaphore.Give());

  ASSERT_TRUE(semaphore.Give());

  ASSERT_FALSE(semaphore.Give());

  // После увеличения счётчика семафора до максимального значения взять семафор
  // можно только 3 раза.
  ASSERT_TRUE(semaphore.Take());

  ASSERT_TRUE(semaphore.Take());

  ASSERT_TRUE(semaphore.Take());

  ASSERT_FALSE(semaphore.Take());
}

TEST(BinaryTestingSemaphore, MultipleGiveAndTake) {
  paraos::BinaryTestingSemaphore binary_semaphore{};

  ASSERT_FALSE(binary_semaphore.Take());

  ASSERT_TRUE(binary_semaphore.Give());

  ASSERT_FALSE(binary_semaphore.Give());

  ASSERT_TRUE(binary_semaphore.Take());

  ASSERT_FALSE(binary_semaphore.Take());
}
