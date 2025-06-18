/// @file test_oneshot_executor.cpp
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
///
/// @copyright (c) 2025 Stilsoft
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
#include <cstdint>

#include "paraos_oneshot_executor.hpp"

namespace {

void MockDelegate() {}

}  // namespace

TEST(OneShotExecutor, Create) {
  // В рамках тестов нет необходимости запускать поток, создаваемый внутри
  // единоразового исполнителя.
  constexpr bool thread_start_flag{false};

  constexpr size_t queue_size{20};

  const paraos::OneShotExecutorAttributes attr;
  const static paraos::OneShotExecutor<queue_size> oneshot_executor{
      attr, thread_start_flag};
}

TEST(OneShotExecutor, EnqueueTooManyDelegates) {
  // В рамках тестов нет необходимости запускать поток, создаваемый внутри
  // единоразового исполнителя.
  constexpr bool thread_start_flag{false};

  constexpr size_t queue_size{2};

  auto mock_delegate = paraos::executor_delegate_type::create<MockDelegate>();

  const paraos::OneShotExecutorAttributes attr;
  paraos::OneShotExecutor<queue_size> oneshot_executor{attr, thread_start_flag};

  ASSERT_TRUE(oneshot_executor.EnqueueDelegate(mock_delegate));
  ASSERT_TRUE(oneshot_executor.EnqueueDelegate(mock_delegate));

  ASSERT_FALSE(oneshot_executor.EnqueueDelegate(mock_delegate));
}
