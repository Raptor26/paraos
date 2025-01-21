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

#include <cstddef>
#include <utility>

#include "paraos_testing_semaphore.hpp"

TEST(TestingSemaphore, CreateWithIncorrectMaxCount) {
  paraos::TestingSemaphoreAttr attrs{};

  attrs.max_count = 0;
  attrs.initial_count = 0;

  const paraos::TestingSemaphore semaphore{attrs};

  ASSERT_FALSE(semaphore);
}

TEST(TestingSemaphore, CreateWithIncorrectInitialCount) {
  constexpr size_t init_count{5};
  paraos::TestingSemaphoreAttr attrs{};

  attrs.max_count = 0;
  attrs.initial_count = init_count;

  const paraos::TestingSemaphore semaphore{attrs};

  ASSERT_FALSE(semaphore);
}

TEST(TestingSemaphore, CreateDefault) {
  const paraos::TestingSemaphoreAttr attrs{};

  const paraos::TestingSemaphore semaphore{attrs};

  ASSERT_TRUE(semaphore);
}

TEST(TestingSemaphore, TakeWithoutGive) {
  paraos::TestingSemaphoreAttr attrs{};

  attrs.initial_count = 0;

  paraos::TestingSemaphore semaphore{attrs};

  ASSERT_TRUE(semaphore);

  ASSERT_FALSE(semaphore.Take());
}

TEST(TestingSemaphore, MultipleGiveAndTake) {
  paraos::TestingSemaphoreAttr attrs{};

  attrs.max_count = 3;
  attrs.initial_count = 0;

  paraos::TestingSemaphore semaphore{attrs};

  ASSERT_TRUE(semaphore);

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

  ASSERT_TRUE(binary_semaphore);

  ASSERT_FALSE(binary_semaphore.Take());

  ASSERT_TRUE(binary_semaphore.Give());

  ASSERT_FALSE(binary_semaphore.Give());

  ASSERT_TRUE(binary_semaphore.Take());

  ASSERT_FALSE(binary_semaphore.Take());
}

TEST(BinaryTestingSemaphore, BinarySemaphoreCreateNotGivenState) {
  paraos::BinaryTestingSemaphore binary_semaphore{};

  ASSERT_TRUE(binary_semaphore);

  ASSERT_FALSE(binary_semaphore.Take(0));
}

TEST(TestingSemaphore, MoveCtor) {
  constexpr size_t max_count{10};
  paraos::TestingSemaphoreAttr attrs{};
  attrs.initial_count = 0;
  attrs.max_count = max_count;

  paraos::TestingSemaphore semaphore_one{attrs};

  semaphore_one.Give();

  const paraos::TestingSemaphore semaphore_two{std::move(semaphore_one)};
}

TEST(TestingSemaphore, MoveAssignment) {
  constexpr size_t sem_one_max_count{10};

  constexpr size_t sem_two_init_count{2};
  constexpr size_t sem_two_max_count{7};

  paraos::TestingSemaphoreAttr attrs{};
  attrs.initial_count = 0;
  attrs.max_count = sem_one_max_count;

  paraos::TestingSemaphore semaphore_one{attrs};

  ASSERT_TRUE(semaphore_one.Give());

  paraos::TestingSemaphoreAttr sem_two_attrs{};
  sem_two_attrs.initial_count = sem_two_init_count;
  sem_two_attrs.max_count = sem_two_max_count;

  paraos::TestingSemaphore semaphore_two{sem_two_attrs};

  semaphore_two = std::move(semaphore_one);
}

TEST(BinaryTestingSemaphore, MoveCtor) {
  paraos::BinaryTestingSemaphore binary_semaphore_one{};

  ASSERT_TRUE(binary_semaphore_one.Give());

  paraos::BinaryTestingSemaphore binary_semaphore_two{
      std::move(binary_semaphore_one)};

  ASSERT_TRUE(binary_semaphore_two.Take());
}

TEST(BinaryTestingSemaphore, MoveAssignment) {
  paraos::BinaryTestingSemaphore binary_semaphore_one{};

  ASSERT_TRUE(binary_semaphore_one.Give());

  paraos::BinaryTestingSemaphore binary_semaphore_two{};

  binary_semaphore_two = std::move(binary_semaphore_one);

  ASSERT_TRUE(binary_semaphore_two.Take());
}
