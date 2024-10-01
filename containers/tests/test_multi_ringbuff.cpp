/// @file test_multi_ringbuff.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
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

#include <limits>
#include <string>

#include "paraos_multi_ringbuff.hpp"

TEST(MultiRingBuff, Create) {
  paraos::MultiRingBuff<10, 128, 5> multi_ring_buff{};
}

TEST(MultiRingBuff, WriteDataToRingBufferThenRead) {
  constexpr std::size_t max_ring_buff_size{128};
  constexpr std::size_t max_ring_buff_numb{5};
  paraos::MultiRingBuff<10, max_ring_buff_size, max_ring_buff_numb>
      multi_ring_buff{};
  std::string str{"Hello world!"};

  constexpr std::size_t cbuff_id{0};
  auto written_bytes_numb = multi_ring_buff.Write(
      cbuff_id, static_cast<const void *>(str.data()), str.size(), 0u);

  ASSERT_EQ(str.size(), written_bytes_numb);

  std::array<std::uint8_t, 128> dst_arr{};

  std::size_t buff_id;
  auto read_bytes_numb = multi_ring_buff.Read(
      buff_id, static_cast<void *>(dst_arr.data()), dst_arr.size(), 0u);

  ASSERT_EQ(written_bytes_numb, read_bytes_numb);
  ASSERT_EQ(cbuff_id, buff_id);
}

TEST(MultiRingBuff, WriteReadMultipleBuffers) {
  constexpr std::size_t max_ring_buff_size{128};
  constexpr std::size_t max_ring_buff_numb{5};
  paraos::MultiRingBuff<10, max_ring_buff_size, max_ring_buff_numb>
      multi_ring_buff{};
  std::string str{"Hello world!"};

  for (std::size_t i = 0; i < max_ring_buff_numb; i++) {
    std::string str_custom = str + " Id:" + std::to_string(i);
    auto written_bytes_numb = multi_ring_buff.Write(
        i, static_cast<const void *>(str_custom.data()), str_custom.size(), 0u);

    ASSERT_EQ(str_custom.size(), written_bytes_numb);
  }

  std::array<std::uint8_t, max_ring_buff_size> dst_arr{};

  for (std::size_t i = 0; i < max_ring_buff_numb; i++) {
    std::size_t buff_id{std::numeric_limits<std::size_t>::max()};
    std::string str_custom = str + " Id:" + std::to_string(i);
    auto read_bytes_numb = multi_ring_buff.Read(
        buff_id, static_cast<void *>(dst_arr.data()), dst_arr.size(), 0u);

    ASSERT_EQ(str_custom.size(), read_bytes_numb);
    ASSERT_EQ(
        0, memcmp(
               static_cast<const void *>(str_custom.data()),
               static_cast<const void *>(dst_arr.data()), read_bytes_numb));
    ASSERT_EQ(i, buff_id);
  }
}

TEST(MultiRingBuff, WriteTwiceReadTwice) {
  constexpr std::size_t max_ring_buff_size{128};
  constexpr std::size_t max_ring_buff_numb{5};
  paraos::MultiRingBuff<10, max_ring_buff_size, max_ring_buff_numb>
      multi_ring_buff{};
  std::string str1{"Hello "};
  std::string str2{"world!"};

  constexpr std::size_t cbuff_id{0};
  std::size_t written_bytes_numb{0};
  {
    written_bytes_numb += multi_ring_buff.Write(
        cbuff_id, static_cast<const void *>(str1.data()), str1.size(), 0u);

    ASSERT_EQ(str1.size(), written_bytes_numb);
  }

  {
    auto written_bytes_numb_second = multi_ring_buff.Write(
        cbuff_id, static_cast<const void *>(str2.data()), str2.size(), 0u);

    written_bytes_numb += written_bytes_numb_second;
    ASSERT_EQ(str2.size(), written_bytes_numb_second);
  }

  std::array<std::uint8_t, 128> dst_arr{};

  std::size_t buff_id;
  auto read_bytes_numb = multi_ring_buff.Read(
      buff_id, static_cast<void *>(dst_arr.data()), dst_arr.size(), 0u);

  ASSERT_STREQ(
      (str1 + str2).c_str(), reinterpret_cast<const char *>(dst_arr.data()));

  ASSERT_EQ(written_bytes_numb, read_bytes_numb);
  ASSERT_EQ(cbuff_id, buff_id);
}
