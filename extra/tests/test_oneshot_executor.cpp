/// @file test_oneshot_executor.cpp
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include "paraos_oneshot_executor.hpp"

namespace {

/// @brief Sample function for delegate creation.
void MockDelegate() {  // NOLINT(llvm-prefer-static-over-anonymous-namespace):
                       // using static triggers misc-use-anonymous-namespace;
                       // keep internal linkage via anonymous namespace.
}

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

  const paraos::one_shot_executor_attr attr;
  const static paraos::one_shot_executor<queue_size> oneshot_executor{
      attr, thread_start_flag};
}

TEST(OneShotExecutor, EnqueueCreatedDelegate) {
  // Thread creation by the one-shot executor is unnecessary for testing.
  constexpr bool thread_start_flag{false};

  constexpr size_t queue_size{2};

  auto mock_delegate = paraos::executor_delegate_type::create<MockDelegate>();

  const paraos::one_shot_executor_attr attr;
  paraos::one_shot_executor<queue_size> oneshot_executor{
      attr, thread_start_flag};

  // Enqueue the pre-created delegate.
  ASSERT_TRUE(oneshot_executor.enqueue_delegate(mock_delegate));

  static TestClassMock test_class{};

  auto class_method_delegate = paraos::executor_delegate_type::create<
      TestClassMock, &TestClassMock::DoSomething>(test_class);

  ASSERT_TRUE(oneshot_executor.enqueue_delegate(class_method_delegate));
}

TEST(OneShotExecutor, AutoCreateAndEnqueueDelegate) {
  // Thread creation by the one-shot executor is unnecessary for testing.
  constexpr bool thread_start_flag{false};

  constexpr size_t queue_size{2};

  const paraos::one_shot_executor_attr attr;
  paraos::one_shot_executor<queue_size> oneshot_executor{
      attr, thread_start_flag};

  static TestClassMock test_class{};

  // Enqueue an automatically created delegate from a public class method.
  ASSERT_TRUE(
      (oneshot_executor
           .enqueue_delegate<TestClassMock, &TestClassMock::DoSomething>(
               test_class)));

  // Enqueue an automatically created delegate from a free function.
  ASSERT_TRUE((oneshot_executor.enqueue_delegate<MockDelegate>()));
}

TEST(OneShotExecutor, EnqueueTooManyDelegates) {
  // Thread creation by the one-shot executor is unnecessary for testing.
  constexpr bool thread_start_flag{false};

  constexpr size_t queue_size{2};

  auto mock_delegate = paraos::executor_delegate_type::create<MockDelegate>();

  const paraos::one_shot_executor_attr attr;
  paraos::one_shot_executor<queue_size> oneshot_executor{
      attr, thread_start_flag};

  ASSERT_TRUE(oneshot_executor.enqueue_delegate(mock_delegate));
  ASSERT_TRUE(oneshot_executor.enqueue_delegate(mock_delegate));

  // Verify queue capacity limits are enforced.
  ASSERT_FALSE(oneshot_executor.enqueue_delegate(mock_delegate));
}
