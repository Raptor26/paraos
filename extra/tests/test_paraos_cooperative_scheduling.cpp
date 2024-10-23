/// @file test_paraos_cooperative_scheduling_.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
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

#include "paraos_thread_cooperative_scheduling.hpp"

TEST(Cooperative, Create) {
  constexpr std::size_t max_task_numb{2};

  paraos::CooperativeSchedulingAttr attr;
  attr.is_need_loop = false;
  attr.is_need_start = false;
  paraos::CooperativeScheduling<max_task_numb> cooperative{attr};
}

TEST(Cooperative, TryPutOverflowTasks) {
  constexpr std::size_t max_task_numb{2};
  paraos::CooperativeSchedulingAttr attr;
  attr.is_need_loop = false;
  attr.is_need_start = false;
  paraos::CooperativeScheduling<max_task_numb> cooperative{attr};

  struct test_task_t : public etl::task {
    test_task_t() : etl::task{1} {}
    uint32_t task_request_work() const override { return 0; }
    void task_process_work() override {}
  };

  test_task_t test_task1;

  ASSERT_TRUE(cooperative.AddTask(test_task1));
  ASSERT_TRUE(cooperative.AddTask(test_task1));

  // No more space in task list.
  ASSERT_FALSE(cooperative.AddTask(test_task1));
}