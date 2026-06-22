/// @file test_paraos_cooperative_scheduling.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include "etl/task.h"
#include "paraos_thread_cooperative_scheduling.hpp"

TEST(Cooperative, Create) {
  constexpr std::size_t max_task_numb{2};
  // В рамках тестов нет необходимости запускать поток, создаваемый внутри
  // кооперативного планировщика.
  constexpr bool thread_start_flag{false};

  const paraos::CooperativeSchedulingAttr attr;
  const static paraos::CooperativeScheduling<max_task_numb> cooperative{
      attr, thread_start_flag};
}

TEST(Cooperative, TryPutOverflowTasks) {
  constexpr std::size_t max_task_numb{2};
  // В рамках тестов нет необходимости запускать поток, создаваемый внутри
  // кооперативного планировщика.
  constexpr bool thread_start_flag{false};

  const paraos::CooperativeSchedulingAttr attr;
  static paraos::CooperativeScheduling<max_task_numb> cooperative{
      attr, thread_start_flag};

  struct test_task_t : public etl::task {
    test_task_t() : etl::task{1} {}
    [[nodiscard]] auto task_request_work() const -> uint32_t override {
      return 0;
    }
    void task_process_work() override {}
  };

  test_task_t test_task1;

  ASSERT_TRUE(cooperative.AddTask(test_task1));
  ASSERT_TRUE(cooperative.AddTask(test_task1));

  // AddTask() throw exception, freeRTOS without start scheduler not support
  // throw exceptions.
#ifndef PARAOS_LIKE_FREERTOS
  // No more space in task list.
  ASSERT_FALSE(cooperative.AddTask(test_task1));
#endif
}