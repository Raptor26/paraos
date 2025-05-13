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

#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>

#include "paraos_message_buffer.hpp"

constexpr std::size_t thread_delay{0};

TEST(Message, Example) {
  // Max message numb for contained in buffer in same time.
  constexpr std::size_t buffer_size{2};

  // Create buffer
  paraos::MessageBuffer<buffer_size> buff;

  // For example, we want write next strings:
  std::string str1{"Hello"};
  std::string str2{" world!"};

  {
    // Alloc memory with null terminate symbol.
    auto message1 = buff.Alloc(str1.length() + 1U);

    // Before any action check container validation.
    if (message1) {
      // Copy data in container.
      memcpy(message1.Data(), str1.data(), message1.Size());
    }

    // when message1 leave scope, message1 calls dtor and data automatically
    // will push in buffer. Be careful: If other thread full queue buffer
    // between buff.Alloc() and message leave scope, message1 will not pushed in
    // buffer. message1 will miss, but always resources will correctly free (no
    // leak memory).
  }

  {
    // Alloc memory with null terminate symbol.
    auto message2 = buff.Alloc(str2.length() + 1U);

    // Before any action check container validation.
    if (message2) {
      // Copy data in container.
      memcpy(message2.Data(), str2.data(), message2.Size());

      // Force push message in buffer.
      // Be careful: If other thread full queue buffer between buff.Alloc() and
      // TryPush(), message1 will not pushed in buffer. message2
      // will miss, but always resources will correctly free (no leak memory).
      auto is_message_pushed = message2.TryPush();

      if (!is_message_pushed) {
        // No space in queue. Message don't pushed in buffer.
      }
    }

    // when message1 leave scope, message1 calls dtor. Because user call
    // TryPush() above, message2 already in buffer and dtor don't push this
    // message again.
  }

  // Let's try read data form buffer.

  constexpr std::size_t timeout_ms{0};

  // Read first string.
  {
    // For example, set zero timeout above, but if set non zero value, thread
    // will wait time with timeout_ms respect when data in buffer will
    // available. Useful in multithread programming.
    auto read = buff.Pop(timeout_ms);

    // Always check if message was read.
    if (read) {
      // Print first string.
      std::cout << reinterpret_cast<char *>(read->Data());
    }

    // read automatically free resources when exit from scope visible.
  }

  // Read second string.
  {
    auto read = buff.Pop(timeout_ms);

    // Always check if message was read.
    if (read) {
      // Print first string.
      std::cout << reinterpret_cast<char *>(read->Data()) << "\n";
    }

    // read automatically free resources when exit from scope visible.
  }
}

TEST(Message, Create) {
  const paraos::MessageBuffer<2> buff;
  ASSERT_TRUE(buff);
}

TEST(Message, PushThenPop) {
  paraos::MessageBuffer<2> buff;
  ASSERT_TRUE(buff);

  constexpr double val{12};
  {
    auto message = buff.Alloc(sizeof(val));

    auto *vector = reinterpret_cast<double *>(message.Data());
    *vector = val;
  }

  auto message = buff.Pop(thread_delay);

  auto *vector = reinterpret_cast<double *>(message->Data());
  EXPECT_NEAR(val, *vector, 0.001);
}

TEST(Message, PushToFull) {
  const paraos::MessageBuffer<2> buff;
  ASSERT_TRUE(buff);

  constexpr double val{12};
  {
    auto message = buff.Alloc(sizeof(val));
    ASSERT_TRUE(message);
    ASSERT_TRUE(buff.IsEmpty());
  }

  {
    auto message = buff.Alloc(sizeof(val));
    ASSERT_TRUE(message);
  }

  {
    auto message = buff.Alloc(sizeof(val));
    ASSERT_FALSE(message.TryPush());
  }
}

TEST(Message, CopyCtor) {
  paraos::MessageBuffer<2> buff;
  ASSERT_TRUE(buff);

  constexpr double val{12};
  {
    auto message = buff.Alloc(sizeof(val));
    ASSERT_TRUE(message);
    ASSERT_TRUE(buff.IsEmpty());

    auto *vector = reinterpret_cast<double *>(message.Data());
    *vector = val;
  }

  auto received_message = buff.Pop(0U);
  ASSERT_TRUE(received_message);
  auto &received_message_copy = received_message.value();

  auto *vector = reinterpret_cast<double *>(received_message_copy.Data());
  EXPECT_NEAR(val, *vector, 0.001);
}

TEST(Message, MoveCtor) {
  paraos::MessageBuffer<2> buff;
  ASSERT_TRUE(buff);

  constexpr double val{12};
  {
    auto message = buff.Alloc(sizeof(val));
    ASSERT_TRUE(message);
    ASSERT_TRUE(buff.IsEmpty());

    auto *vector = reinterpret_cast<double *>(message.Data());
    *vector = val;
  }

  auto received_message = buff.Pop(0U);
  ASSERT_TRUE(received_message);
  auto received_message_copy = std::move(received_message.value());

  auto *vector = reinterpret_cast<double *>(received_message_copy.Data());
  EXPECT_NEAR(val, *vector, 0.001);
}

TEST(Message, PushButForceFree) {
  const paraos::MessageBuffer<2> buff;
  ASSERT_TRUE(buff);

  ASSERT_TRUE(buff.IsEmpty());
  constexpr double val{12};

  {
    auto message = buff.Alloc(sizeof(val));
    message.Free();
  }

  ASSERT_TRUE(buff.IsEmpty());
}
