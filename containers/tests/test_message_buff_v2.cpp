#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <set>
#include <string>

#include "paraos_message_buffer_v2.hpp"

constexpr std::size_t thread_delay{0};

TEST(Message, Create) {
  paraos::v2::MessageBuffer buff{2};
  ASSERT_TRUE(buff);
}

TEST(Message, CreateEmpty) {
  paraos::v2::MessageBuffer buff{0};
  ASSERT_FALSE(buff);
}

TEST(Message, PushThenPop) {
  paraos::v2::MessageBuffer buff{2};
  ASSERT_TRUE(buff);

  constexpr double val{12};
  {
    auto message = buff.Alloc(sizeof(val), thread_delay);

    auto *vector = static_cast<double *>(message.Addr());
    *vector = val;
  }

  auto message = buff.Pop(thread_delay);

  auto *vector = static_cast<double *>(message.value().Addr());
  EXPECT_NEAR(val, *vector, 0.001);
}

TEST(Message, PushToFull) {
  paraos::v2::MessageBuffer buff{2};
  ASSERT_TRUE(buff);

  constexpr double val{12};
  {
    auto message = buff.Alloc(sizeof(val), thread_delay);
    ASSERT_TRUE(message);
    ASSERT_TRUE(buff.IsEmpty());
  }

  {
    auto message = buff.Alloc(sizeof(val), thread_delay);
    ASSERT_TRUE(message);
  }

  {
    auto message = buff.Alloc(sizeof(val), thread_delay);
    ASSERT_FALSE(message.Push());
  }
}

TEST(Message, PushButForceFree) {
  paraos::v2::MessageBuffer buff{2};
  ASSERT_TRUE(buff);

  ASSERT_TRUE(buff.IsEmpty());
  constexpr double val{12};

  {
    auto message = buff.Alloc(sizeof(val), thread_delay);
    message.Free();
  }

  ASSERT_TRUE(buff.IsEmpty());
}