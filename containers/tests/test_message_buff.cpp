/// @file test_message_buff.cpp
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

#include <array>
#include <memory>
#include <set>
#include <string>

#include "paraos_message_buffer.hpp"

constexpr std::size_t thread_delay{0};

TEST(Message, Create) {
  paraos::MessageBuffer buff{2};
  ASSERT_TRUE(buff);
}

TEST(Message, CreateEmpty) {
  paraos::MessageBuffer buff{0};
  ASSERT_FALSE(buff);
}

TEST(Message, PushThenPop) {
  paraos::MessageBuffer buff{2};
  ASSERT_TRUE(buff);

  constexpr double val{12};
  {
    auto message = buff.Alloc(sizeof(val), thread_delay);

    auto *vector = static_cast<double *>(message.Addr());
    *vector = val;
  }

  auto message = buff.Pop(thread_delay);

  auto *vector = static_cast<double *>(message->Addr());
  EXPECT_NEAR(val, *vector, 0.001);
}

TEST(Message, PushToFull) {
  paraos::MessageBuffer buff{2};
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
  paraos::MessageBuffer buff{2};
  ASSERT_TRUE(buff);

  ASSERT_TRUE(buff.IsEmpty());
  constexpr double val{12};

  {
    auto message = buff.Alloc(sizeof(val), thread_delay);
    message.Free();
  }

  ASSERT_TRUE(buff.IsEmpty());
}
