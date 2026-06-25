/// @file test_message_buff.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
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
  paraos::message_buffer<buffer_size> buff;

  // For example, we want write next strings:
  std::string str1{"Hello"};
  std::string str2{" world!"};

  {
    // Alloc memory with null terminate symbol.
    auto message1 = buff.alloc(str1.length() + 1U);

    // Before any action check container validation.
    if (message1) {
      // Copy data in container.
      memcpy(message1.data(), str1.data(), message1.size());
    }

    // when message1 leave scope, message1 calls dtor and data automatically
    // will push in buffer. Be careful: If other thread full queue buffer
    // between buff.alloc() and message leave scope, message1 will not pushed in
    // buffer. message1 will miss, but always resources will correctly free (no
    // leak memory).
  }

  {
    // Alloc memory with null terminate symbol.
    auto message2 = buff.alloc(str2.length() + 1U);

    // Before any action check container validation.
    if (message2) {
      // Copy data in container.
      memcpy(message2.data(), str2.data(), message2.size());

      // Force push message in buffer.
      // Be careful: If other thread full queue buffer between buff.alloc() and
      // try_push(), message1 will not pushed in buffer. message2
      // will miss, but always resources will correctly free (no leak memory).
      auto is_message_pushed = message2.try_push();

      if (!is_message_pushed) {
        // No space in queue. Message don't pushed in buffer.
      }
    }

    // when message1 leave scope, message1 calls dtor. Because user call
    // try_push() above, message2 already in buffer and dtor don't push this
    // message again.
  }

  // Let's try read data form buffer.

  constexpr std::size_t timeout_ms{0};

  // Read first string.
  {
    // For example, set zero timeout above, but if set non zero value, thread
    // will wait time with timeout_ms respect when data in buffer will
    // available. Useful in multithread programming.
    auto read = buff.pop(timeout_ms);

    // Always check if message was read.
    if (read) {
      // Print first string.
      std::cout << reinterpret_cast<char*>(read->data());
    }

    // read automatically free resources when exit from scope visible.
  }

  // Read second string.
  {
    auto read = buff.pop(timeout_ms);

    // Always check if message was read.
    if (read) {
      // Print first string.
      std::cout << reinterpret_cast<char*>(read->data()) << "\n";
    }

    // read automatically free resources when exit from scope visible.
  }
}

TEST(Message, Create) {
  const paraos::message_buffer<2> buff;
  ASSERT_TRUE(buff);
}

TEST(Message, PushThenPop) {
  paraos::message_buffer<2> buff;
  ASSERT_TRUE(buff);

  constexpr double val{12};
  {
    auto message = buff.alloc(sizeof(val));

    auto* vector = reinterpret_cast<double*>(message.data());
    *vector = val;
  }

  auto message = buff.pop(thread_delay);

  auto* vector = reinterpret_cast<double*>(message->data());
  EXPECT_NEAR(val, *vector, 0.001);
}

TEST(Message, PushToFull) {
  const paraos::message_buffer<2> buff;
  ASSERT_TRUE(buff);

  constexpr double val{12};
  {
    auto message = buff.alloc(sizeof(val));
    ASSERT_TRUE(message);
    ASSERT_TRUE(buff.is_empty());
  }

  {
    auto message = buff.alloc(sizeof(val));
    ASSERT_TRUE(message);
  }

  {
    auto message = buff.alloc(sizeof(val));
    ASSERT_FALSE(message.try_push());
  }
}

TEST(Message, CopyCtor) {
  paraos::message_buffer<2> buff;
  ASSERT_TRUE(buff);

  constexpr double val{12};
  {
    auto message = buff.alloc(sizeof(val));
    ASSERT_TRUE(message);
    ASSERT_TRUE(buff.is_empty());

    auto* vector = reinterpret_cast<double*>(message.data());
    *vector = val;
  }

  auto received_message = buff.pop(0U);
  ASSERT_TRUE(received_message);
  auto& received_message_copy = received_message.value();

  auto* vector = reinterpret_cast<double*>(received_message_copy.data());
  EXPECT_NEAR(val, *vector, 0.001);
}

TEST(Message, MoveCtor) {
  paraos::message_buffer<2> buff;
  ASSERT_TRUE(buff);

  constexpr double val{12};
  {
    auto message = buff.alloc(sizeof(val));
    ASSERT_TRUE(message);
    ASSERT_TRUE(buff.is_empty());

    auto* vector = reinterpret_cast<double*>(message.data());
    *vector = val;
  }

  auto received_message = buff.pop(0U);
  ASSERT_TRUE(received_message);
  auto received_message_copy = std::move(received_message.value());

  auto* vector = reinterpret_cast<double*>(received_message_copy.data());
  EXPECT_NEAR(val, *vector, 0.001);
}

TEST(Message, PushButForceFree) {
  const paraos::message_buffer<2> buff;
  ASSERT_TRUE(buff);

  ASSERT_TRUE(buff.is_empty());
  constexpr double val{12};

  {
    auto message = buff.alloc(sizeof(val));
    message.free();
  }

  ASSERT_TRUE(buff.is_empty());
}
