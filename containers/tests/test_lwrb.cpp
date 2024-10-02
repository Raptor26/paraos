/// @file test_lwrb.cpp
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
#include <string>

#include "paraos_ringbuff.hpp"

TEST(RingBuff, Create) {
  try {
    paraos::RingBuff<char, 2> ring_buff;
  } catch (paraos::ringbuff_ctor_error &e) {
    FAIL() << "paraos::RingBuff can't throw exception" << std::endl;
  }
}

TEST(RingBuff, WriteThenRead) {
  std::string src{"Hello World!"};
  constexpr std::size_t ringbuff_size_in_bytes{100};

  paraos::RingBuff<char, ringbuff_size_in_bytes> ring_buff;

  // Free bytes less at one bytes from size.
  ASSERT_EQ(ringbuff_size_in_bytes - 1, ring_buff.Free());

  auto written_bytes_numb =
      ring_buff.Write(static_cast<const void *>(src.c_str()), src.size());
  ASSERT_EQ(src.size(), written_bytes_numb);
  ASSERT_EQ(src.size(), ring_buff.Size());

  std::array<char, 100> dst_;
  auto read_bytes_numb = ring_buff.Read(dst_.data(), dst_.size());
  ASSERT_EQ(written_bytes_numb, read_bytes_numb);

  ASSERT_EQ(
      0, memcmp(
             static_cast<const void *>(src.data()),
             static_cast<const void *>(dst_.data()), src.size()));

  // All date read. Free bytes less at one bytes from size.
  ASSERT_EQ(ringbuff_size_in_bytes - 1, ring_buff.Free());
}

TEST(RingBuff, WriteThenReadIterator) {
  constexpr std::size_t ringbuff_size_in_bytes{100};

  // User std::array instead std::stirng or check simple iterator.
  constexpr std::array<char, ringbuff_size_in_bytes> src{"Hello World!"};
  const auto str_len_without_null = strlen(src.data());

  paraos::RingBuff<char, ringbuff_size_in_bytes> ring_buff;

  auto written_bytes_numb =
      ring_buff.Write(src.begin(), src.begin() + str_len_without_null);
  ASSERT_EQ(str_len_without_null, written_bytes_numb);
  ASSERT_EQ(str_len_without_null, ring_buff.Size());

  std::array<char, 100> dst_;
  auto read_bytes_numb = ring_buff.Read(dst_.begin(), dst_.end());
  ASSERT_EQ(written_bytes_numb, read_bytes_numb);

  ASSERT_EQ(
      0, memcmp(
             static_cast<const void *>(src.data()),
             static_cast<const void *>(dst_.data()), str_len_without_null));
}

TEST(RingBuff, Clear) {
  std::string src{"Hello World!"};
  constexpr std::size_t ringbuff_size_in_bytes{100};
  paraos::RingBuff<char, ringbuff_size_in_bytes> ring_buff;

  auto written_bytes_numb =
      ring_buff.Write(static_cast<const void *>(src.data()), src.size());

  ASSERT_EQ(written_bytes_numb, ring_buff.Size());

  ring_buff.Clear();
  ASSERT_EQ(0u, ring_buff.Size());
}

TEST(RingBuff, Skip) {
  std::string src{"Hello World!"};
  constexpr std::size_t ringbuff_size_in_bytes{100};
  paraos::RingBuff<char, ringbuff_size_in_bytes> ring_buff;

  auto written_bytes_numb =
      ring_buff.Write(static_cast<const void *>(src.data()), src.size());

  ASSERT_EQ(written_bytes_numb, ring_buff.Size());

  constexpr std::size_t skip_need{10};
  auto skip_bytes = ring_buff.Skip(skip_need);
  ASSERT_EQ(skip_need, skip_bytes);

  ASSERT_EQ(written_bytes_numb - skip_need, ring_buff.Size());
}

TEST(RingBuff, WriteSpanThenReadSpan) {
  std::string src{"Hello World!"};
  constexpr std::size_t ringbuff_size_in_bytes{100};

  paraos::RingBuff<char, ringbuff_size_in_bytes> ring_buff;

  auto written_bytes_numb = ring_buff.Write(src);
  ASSERT_EQ(src.size(), written_bytes_numb);
  ASSERT_EQ(src.size(), ring_buff.Size());

  std::array<char, 100> dst_;
  auto read_bytes_numb = ring_buff.Read(dst_);
  ASSERT_EQ(written_bytes_numb, read_bytes_numb);

  ASSERT_EQ(
      0, memcmp(
             static_cast<const void *>(src.data()),
             static_cast<const void *>(dst_.data()), src.size()));

  // All data was read. Free bytes must be one byte less than buffer size.
  ASSERT_EQ(ringbuff_size_in_bytes - 1, ring_buff.Free());
}
