/// @file test_oneshot_executor.cpp
/// @author Egor Vyhodcev (vyhodcev@internet.ru)
///
/// @copyright (c) 2025 Stilsoft
///
/// MIT License:
///
/// Permission is granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to
/// deal in the Software without restriction, including the rights to use,
/// copy, modify, merge, publish, distribute, sublicense, and/or sell copies
/// of the Software, and to permit persons to whom the Software is furnished
/// to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included
/// in all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
/// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
/// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include "paraos_oneshot_executor.hpp"

namespace {

/// @brief Sample function for delegate creation.
void MockDelegate() {}

// NOLINTBEGIN(*-special-member-functions)
class TestClassMock {
 public:
  TestClassMock() = default;
  ~TestClassMock() = default;

  /// Public method used for delegate creation.
  void DoSomething() {}
};
// NOLINTEND(*-special-member-functions)

}  // namespace

TEST(OneShotExecutor, Create) {
  // Thread creation by the one-shot executor is unnecessary for testing.
  constexpr bool thread_start_flag{false};

  constexpr size_t queue_size{20};

  const paraos::OneShotExecutorAttributes attr;
  const static paraos::OneShotExecutor<queue_size> oneshot_executor{
      attr, thread_start_flag};
}

TEST(OneShotExecutor, EnqueueCreatedDelegate) {
  // Thread creation by the one-shot executor is unnecessary for testing.
  constexpr bool thread_start_flag{false};

  constexpr size_t queue_size{2};

  auto mock_delegate = paraos::executor_delegate_type::create<MockDelegate>();

  const paraos::OneShotExecutorAttributes attr;
  paraos::OneShotExecutor<queue_size> oneshot_executor{attr, thread_start_flag};

  // Enqueue the pre-created delegate.
  ASSERT_TRUE(oneshot_executor.EnqueueDelegate(mock_delegate));

  static TestClassMock test_class{};

  auto class_method_delegate = paraos::executor_delegate_type::create<
      TestClassMock, &TestClassMock::DoSomething>(test_class);

  ASSERT_TRUE(oneshot_executor.EnqueueDelegate(class_method_delegate));
}

TEST(OneShotExecutor, AutoCreateAndEnqueueDelegate) {
  // Thread creation by the one-shot executor is unnecessary for testing.
  constexpr bool thread_start_flag{false};

  constexpr size_t queue_size{2};

  const paraos::OneShotExecutorAttributes attr;
  paraos::OneShotExecutor<queue_size> oneshot_executor{attr, thread_start_flag};

  static TestClassMock test_class{};

  // Enqueue an automatically created delegate from a public class method.
  ASSERT_TRUE((oneshot_executor
                   .EnqueueDelegate<TestClassMock, &TestClassMock::DoSomething>(
                       test_class)));

  // Enqueue an automatically created delegate from a free function.
  ASSERT_TRUE((oneshot_executor.EnqueueDelegate<MockDelegate>()));
}

TEST(OneShotExecutor, EnqueueTooManyDelegates) {
  // Thread creation by the one-shot executor is unnecessary for testing.
  constexpr bool thread_start_flag{false};

  constexpr size_t queue_size{2};

  auto mock_delegate = paraos::executor_delegate_type::create<MockDelegate>();

  const paraos::OneShotExecutorAttributes attr;
  paraos::OneShotExecutor<queue_size> oneshot_executor{attr, thread_start_flag};

  ASSERT_TRUE(oneshot_executor.EnqueueDelegate(mock_delegate));
  ASSERT_TRUE(oneshot_executor.EnqueueDelegate(mock_delegate));

  // Verify queue capacity limits are enforced.
  ASSERT_FALSE(oneshot_executor.EnqueueDelegate(mock_delegate));
}