/// @file test_testing_semaphore.cpp
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <gtest/gtest.h>

#include <cstddef>
#include <utility>

#include "paraos_testing_semaphore.hpp"

TEST(TestingSemaphore, CreateWithIncorrectMaxCount) {
  paraos::testing_semaphore_attr attrs{};

  attrs.max_count = 0;
  attrs.initial_count = 0;

  const paraos::testing_semaphore semaphore{attrs};

  ASSERT_FALSE(semaphore);
}

TEST(TestingSemaphore, CreateWithIncorrectInitialCount) {
  constexpr size_t init_count{5};
  paraos::testing_semaphore_attr attrs{};

  attrs.max_count = 0;
  attrs.initial_count = init_count;

  const paraos::testing_semaphore semaphore{attrs};

  ASSERT_FALSE(semaphore);
}

TEST(TestingSemaphore, CreateDefault) {
  const paraos::testing_semaphore_attr attrs{};

  const paraos::testing_semaphore semaphore{attrs};

  ASSERT_TRUE(semaphore);
}

TEST(TestingSemaphore, TakeWithoutGive) {
  paraos::testing_semaphore_attr attrs{};

  attrs.initial_count = 0;

  paraos::testing_semaphore semaphore{attrs};

  ASSERT_TRUE(semaphore);

  ASSERT_FALSE(semaphore.take());
}

TEST(TestingSemaphore, MultipleGiveAndTake) {
  paraos::testing_semaphore_attr attrs{};

  attrs.max_count = 3;
  attrs.initial_count = 0;

  paraos::testing_semaphore semaphore{attrs};

  ASSERT_TRUE(semaphore);

  // Поскольку максимальное значение счётчика семафора равно 3, отдать семафор
  // можно не более 3 раз.
  ASSERT_TRUE(semaphore.give());

  ASSERT_TRUE(semaphore.give());

  ASSERT_TRUE(semaphore.give());

  ASSERT_FALSE(semaphore.give());

  // После увеличения счётчика семафора до максимального значения взять семафор
  // можно только 3 раза.
  ASSERT_TRUE(semaphore.take());

  ASSERT_TRUE(semaphore.take());

  ASSERT_TRUE(semaphore.take());

  ASSERT_FALSE(semaphore.take());
}

TEST(BinaryTestingSemaphore, MultipleGiveAndTake) {
  paraos::binary_testing_semaphore binary_semaphore{};

  ASSERT_TRUE(binary_semaphore);

  ASSERT_FALSE(binary_semaphore.take());

  ASSERT_TRUE(binary_semaphore.give());

  ASSERT_FALSE(binary_semaphore.give());

  ASSERT_TRUE(binary_semaphore.take());

  ASSERT_FALSE(binary_semaphore.take());
}

TEST(BinaryTestingSemaphore, BinarySemaphoreCreateNotGivenState) {
  paraos::binary_testing_semaphore binary_semaphore{};

  ASSERT_TRUE(binary_semaphore);

  ASSERT_FALSE(binary_semaphore.take(0));
}

TEST(TestingSemaphore, MoveCtor) {
  constexpr size_t max_count{10};
  paraos::testing_semaphore_attr attrs{};
  attrs.initial_count = 0;
  attrs.max_count = max_count;

  paraos::testing_semaphore semaphore_one{attrs};

  semaphore_one.give();

  const paraos::testing_semaphore semaphore_two{std::move(semaphore_one)};
}

TEST(TestingSemaphore, MoveAssignment) {
  constexpr size_t sem_one_max_count{10};

  constexpr size_t sem_two_init_count{2};
  constexpr size_t sem_two_max_count{7};

  paraos::testing_semaphore_attr attrs{};
  attrs.initial_count = 0;
  attrs.max_count = sem_one_max_count;

  paraos::testing_semaphore semaphore_one{attrs};

  ASSERT_TRUE(semaphore_one.give());

  paraos::testing_semaphore_attr sem_two_attrs{};
  sem_two_attrs.initial_count = sem_two_init_count;
  sem_two_attrs.max_count = sem_two_max_count;

  paraos::testing_semaphore semaphore_two{sem_two_attrs};

  semaphore_two = std::move(semaphore_one);
}

TEST(BinaryTestingSemaphore, MoveCtor) {
  paraos::binary_testing_semaphore binary_semaphore_one{};

  ASSERT_TRUE(binary_semaphore_one.give());

  paraos::binary_testing_semaphore binary_semaphore_two{
      std::move(binary_semaphore_one)};

  ASSERT_TRUE(binary_semaphore_two.take());
}

TEST(BinaryTestingSemaphore, MoveAssignment) {
  paraos::binary_testing_semaphore binary_semaphore_one{};

  ASSERT_TRUE(binary_semaphore_one.give());

  paraos::binary_testing_semaphore binary_semaphore_two{};

  binary_semaphore_two = std::move(binary_semaphore_one);

  ASSERT_TRUE(binary_semaphore_two.take());
}

TEST(AlwaysTrueSemaphore, Create) {
  const paraos::always_true_semaphore true_sem{};

  ASSERT_TRUE(true_sem);
}

TEST(AlwaysTrueSemaphore, TakeMultipleTimesWithoutGive) {
  const paraos::always_true_semaphore true_sem{};

  ASSERT_TRUE(true_sem.take());

  ASSERT_TRUE(true_sem.take());

  ASSERT_TRUE(true_sem.take());

  ASSERT_TRUE(true_sem.give());

  ASSERT_TRUE(true_sem.give());
}

TEST(AlwaysTrueSemaphore, MoveCtor) {
  paraos::always_true_semaphore true_sem_one{};

  const paraos::always_true_semaphore true_sem_two{std::move(true_sem_one)};

  ASSERT_TRUE(true_sem_two.take());
}

TEST(AlwaysTrueSemaphore, MoveAssignment) {
  paraos::always_true_semaphore true_sem_one{};

  paraos::always_true_semaphore true_sem_two{};

  true_sem_two = std::move(true_sem_one);

  ASSERT_TRUE(true_sem_two.take());
}
