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

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "paraos_multi_ringbuff.hpp"
#include "paraos_ringbuff.hpp"

TEST(MultiRingBuff, Create) {
  constexpr std::size_t queue_size{10};
  constexpr std::size_t max_ring_buff_size{128};

  const paraos::MultiRingBuff<
      queue_size, std::uint8_t,
      paraos::RingBuff<std::uint8_t, max_ring_buff_size>>
      multi_ring_buff{};
  ASSERT_EQ(1U, multi_ring_buff.GetBuffNumb());
}

TEST(MultiRingBuff, WriteDataToRingBufferThenRead) {
  constexpr std::size_t queue_size{10};
  constexpr std::size_t max_ring_buff_size{128};
  constexpr std::size_t array_size{128};

  paraos::MultiRingBuff<
      queue_size, char, paraos::RingBuff<char, max_ring_buff_size>,
      paraos::RingBuff<char, max_ring_buff_size>>
      multi_ring_buff{};
  std::string str{"Hello world!"};

  constexpr std::size_t cbuff_id{0};
  const std::size_t expect_written_bytes{str.size()};
  auto is_written_successful =
      multi_ring_buff.TryWrite(cbuff_id, str.data(), str.size());

  ASSERT_TRUE(is_written_successful);

  std::array<std::uint8_t, array_size> dst_arr{};

  std::size_t buff_id;
  auto read_bytes_numb = multi_ring_buff.Read(
      buff_id, static_cast<void*>(dst_arr.data()), dst_arr.size(), 0U);

  ASSERT_EQ(expect_written_bytes, read_bytes_numb);
  ASSERT_EQ(cbuff_id, buff_id);
}

// NOLINTBEGIN(readability-function-cognitive-complexity)
TEST(MultiRingBuff, WriteReadMultipleBuffers) {
  constexpr std::size_t queue_size{10};
  constexpr std::size_t max_ring_buff_size{128};
  constexpr std::size_t max_ring_buff_numb{5};
  paraos::MultiRingBuff<
      queue_size, char, paraos::RingBuff<char, max_ring_buff_size>,
      paraos::RingBuff<char, max_ring_buff_size>,
      paraos::RingBuff<char, max_ring_buff_size>,
      paraos::RingBuff<char, max_ring_buff_size>,
      paraos::RingBuff<char, max_ring_buff_size>>
      multi_ring_buff{};

  ASSERT_EQ(max_ring_buff_numb, multi_ring_buff.GetBuffNumb());

  const std::string str{"Hello world!"};

  for (std::size_t i = 0; i < max_ring_buff_numb; i++) {
    std::string str_custom = str + " Id:" + std::to_string(i);

    ASSERT_TRUE(
        multi_ring_buff.TryWrite(i, str_custom.data(), str_custom.size()));
  }

  std::array<std::uint8_t, max_ring_buff_size> dst_arr{};

  for (std::size_t i = 0; i < max_ring_buff_numb; i++) {
    std::size_t buff_id{std::numeric_limits<std::size_t>::max()};
    std::string str_custom = str + " Id:" + std::to_string(i);
    auto read_bytes_numb = multi_ring_buff.Read(
        buff_id, reinterpret_cast<void*>(dst_arr.data()), dst_arr.size(), 0U);

    ASSERT_EQ(str_custom.size(), read_bytes_numb);
    ASSERT_EQ(
        0, memcmp(
               reinterpret_cast<const void*>(str_custom.data()),
               reinterpret_cast<const void*>(dst_arr.data()), read_bytes_numb));
    ASSERT_EQ(i, buff_id);
  }
}
// NOLINTEND(readability-function-cognitive-complexity)

TEST(MultiRingBuff, WriteTwiceReadTwice) {
  constexpr std::size_t queue_size{10};
  constexpr std::size_t max_ring_buff_size{128};
  constexpr std::size_t array_size{128};

  paraos::MultiRingBuff<
      queue_size, char, paraos::RingBuff<char, max_ring_buff_size>,
      paraos::RingBuff<char, max_ring_buff_size>>
      multi_ring_buff{};
  std::string str1{"Hello "};
  std::string str2{"world!"};

  constexpr std::size_t cbuff_id{0};
  std::size_t expected_written_bytes{0};
  {
    expected_written_bytes += str1.size();
    auto is_write_successful =
        multi_ring_buff.TryWrite(cbuff_id, str1.data(), str1.size());

    ASSERT_TRUE(is_write_successful);
  }

  {
    expected_written_bytes += str2.size();
    auto is_write_successful =
        multi_ring_buff.TryWrite(cbuff_id, str2.data(), str2.size());

    ASSERT_TRUE(is_write_successful);
  }

  std::array<std::uint8_t, array_size> dst_arr{};

  std::size_t buff_id;
  auto read_bytes_numb = multi_ring_buff.Read(
      buff_id, static_cast<void*>(dst_arr.data()), dst_arr.size(), 0U);

  ASSERT_STREQ(
      (str1 + str2).c_str(), reinterpret_cast<const char*>(dst_arr.data()));

  ASSERT_EQ(expected_written_bytes, read_bytes_numb);
  ASSERT_EQ(cbuff_id, buff_id);
}

TEST(MultiRingBuff, WriteSpanToRingBufferThenRead) {
  constexpr std::size_t queue_size{10};
  constexpr std::size_t max_ring_buff_size{128};
  constexpr std::size_t array_size{128};

  paraos::MultiRingBuff<
      queue_size, uint8_t, paraos::RingBuff<uint8_t, max_ring_buff_size>,
      paraos::RingBuff<uint8_t, max_ring_buff_size>>
      multi_ring_buff{};
  std::string str{"Hello world!"};
  // Каст строки в массив uint8_t для последующего преобразования в span.
  std::vector<uint8_t> myVector(str.begin(), str.end());

  constexpr std::size_t cbuff_id{0};
  const std::size_t expected_written_bytes{myVector.size()};
  auto is_written_successful = multi_ring_buff.TryWrite(cbuff_id, myVector);

  ASSERT_TRUE(is_written_successful);

  std::array<std::uint8_t, array_size> dst_arr{};

  std::size_t buff_id;
  auto read_bytes_numb = multi_ring_buff.Read(buff_id, dst_arr, 0U);

  ASSERT_EQ(expected_written_bytes, read_bytes_numb);
  ASSERT_EQ(cbuff_id, buff_id);
  ASSERT_EQ(
      0, memcmp(
             reinterpret_cast<const void*>(str.data()),
             reinterpret_cast<const void*>(dst_arr.data()), read_bytes_numb));
}

TEST(MultiRingBuff, WriteIteratorThenReadIterator) {
  constexpr std::size_t queue_size{10};
  constexpr std::size_t max_ring_buff_size{128};
  constexpr std::size_t array_size{128};

  paraos::MultiRingBuff<
      queue_size, unsigned char,
      paraos::RingBuff<unsigned char, max_ring_buff_size>,
      paraos::RingBuff<unsigned char, max_ring_buff_size>>
      multi_ring_buff{};

  std::array<unsigned char, array_size> str{"Hello world!"};

  multi_ring_buff.TryWrite(0U, str.begin(), str.end());
}

TEST(MultiRingBuff, WriteVectorIteratorThenRead) {
  constexpr std::size_t queue_size{10};
  constexpr std::size_t max_ring_buff_size{128};
  constexpr std::size_t array_size{128};

  paraos::MultiRingBuff<
      queue_size, std::uint8_t,
      paraos::RingBuff<std::uint8_t, max_ring_buff_size>,
      paraos::RingBuff<std::uint8_t, max_ring_buff_size>>
      multi_ring_buff{};

  const std::string str{"Hello world!"};
  const std::vector<std::uint8_t> data(str.begin(), str.end());

  constexpr std::size_t cbuff_id{0};
  ASSERT_TRUE(multi_ring_buff.TryWrite(cbuff_id, data.begin(), data.end()));

  std::array<std::uint8_t, array_size> dst_arr{};
  std::size_t buff_id;
  const auto read_bytes_numb =
      multi_ring_buff.Read(buff_id, dst_arr.data(), dst_arr.size(), 0U);

  ASSERT_EQ(data.size(), read_bytes_numb);
  ASSERT_EQ(cbuff_id, buff_id);
  ASSERT_EQ(
      0, memcmp(
             reinterpret_cast<const void*>(str.data()),
             reinterpret_cast<const void*>(dst_arr.data()), read_bytes_numb));
}

// NOLINTBEGIN(readability-function-cognitive-complexity)
TEST(MultiRingBuff, TryRead) {
  constexpr std::size_t queue_size{10};
  constexpr std::size_t max_ring_buff_size{128};
  constexpr std::size_t array_size{128};

  paraos::MultiRingBuff<
      queue_size, char, paraos::RingBuff<char, max_ring_buff_size>,
      paraos::RingBuff<char, max_ring_buff_size>>
      multi_ring_buff{};

  const std::vector<std::string> str_arr{{"Hello"}, {" world!"}};

  ASSERT_EQ(multi_ring_buff.GetBuffNumb(), str_arr.size());

  for (std::size_t i = 0; i < multi_ring_buff.GetBuffNumb(); ++i) {
    EXPECT_TRUE(multi_ring_buff.TryWrite(
        i, str_arr.at(i).data(), str_arr.at(i).length()));
  }

  std::array<char, array_size> dst{};
  for (std::size_t i = 0; i < multi_ring_buff.GetBuffNumb(); ++i) {
    std::size_t idx;
    EXPECT_LT(0, multi_ring_buff.TryRead(idx, dst.data(), dst.size()));
    EXPECT_EQ(0, str_arr.at(i).compare(dst.data()));
  }

  // Try read empty buff.
  std::size_t idx;
  EXPECT_EQ(0, multi_ring_buff.TryRead(idx, dst.data(), dst.size()));
}
// NOLINTEND(readability-function-cognitive-complexity)

TEST(MultiRingBuff, TryReadSpan) {
  constexpr std::size_t queue_size{10};
  constexpr std::size_t max_ring_buff_size{128};
  constexpr std::size_t array_size{128};

  paraos::MultiRingBuff<
      queue_size, char, paraos::RingBuff<char, max_ring_buff_size>,
      paraos::RingBuff<char, max_ring_buff_size>>
      multi_ring_buff{};

  std::string str{"Hello world!"};
  EXPECT_TRUE(multi_ring_buff.TryWrite(0U, str.data(), str.length()));

  std::array<char, array_size> dst{};

  std::size_t idx;
  EXPECT_LT(0, multi_ring_buff.TryRead(idx, dst));
}