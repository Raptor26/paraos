/// @file test_lwrb.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "paraos_ringbuff.hpp"

TEST(RingBuff, Create) {
  try {
    const paraos::RingBuff<char, 2> ring_buff;
  } catch (paraos::ringbuff_ctor_error_exception& e) {
    FAIL() << "paraos::RingBuff can't throw exception" << "\n";
  }
}

TEST(RingBuff, Capacity) {
  const paraos::RingBuff<char, 2> ring_buff;
  ASSERT_EQ(2, ring_buff.Capacity());
  ASSERT_EQ(2, ring_buff.Free());
}

TEST(RingBuff, IsEmpty) {
  paraos::RingBuff<char, 2> ring_buff;
  ASSERT_TRUE(ring_buff.IsEmpty());

  const char symb{'h'};
  ASSERT_EQ(sizeof(symb), ring_buff.Write(&symb, sizeof(symb)));
  ASSERT_FALSE(ring_buff.IsEmpty());

  ring_buff.Clear();
  ASSERT_TRUE(ring_buff.IsEmpty());
}

TEST(RingBuff, IsFull) {
  paraos::RingBuff<char, 2> ring_buff;
  const char symb{'h'};

  ASSERT_EQ(sizeof(symb), ring_buff.Write(&symb, sizeof(symb)));
  ASSERT_FALSE(ring_buff.IsFull());

  ASSERT_EQ(sizeof(symb), ring_buff.Write(&symb, sizeof(symb)));
  ASSERT_TRUE(ring_buff.IsFull());
}

TEST(RingBuff, WriteThenRead) {
  std::string src{"Hello World!"};
  constexpr std::size_t ringbuff_size_in_bytes{100};
  constexpr size_t array_size{128};

  paraos::RingBuff<char, ringbuff_size_in_bytes> ring_buff;

  ASSERT_EQ(ringbuff_size_in_bytes, ring_buff.Free());

  auto written_bytes_numb =
      ring_buff.Write(static_cast<const void*>(src.c_str()), src.size());
  ASSERT_EQ(src.size(), written_bytes_numb);
  ASSERT_EQ(src.size(), ring_buff.Size());

  std::array<char, array_size> dst_{0};
  auto read_bytes_numb = ring_buff.Read(dst_.data(), dst_.size());
  ASSERT_EQ(written_bytes_numb, read_bytes_numb);

  ASSERT_EQ(
      0, memcmp(
             static_cast<const void*>(src.data()),
             static_cast<const void*>(dst_.data()), src.size()));

  // All date read.
  ASSERT_EQ(ringbuff_size_in_bytes, ring_buff.Free());
}

TEST(RingBuff, WriteThenReadIterator) {
  constexpr std::size_t ringbuff_size_in_bytes{100};
  constexpr size_t array_size{128};

  // User std::array instead std::stirng or check simple iterator.
  constexpr std::array<char, ringbuff_size_in_bytes> src{"Hello World!"};
  const auto str_len_without_null = strlen(src.data());

  paraos::RingBuff<char, ringbuff_size_in_bytes> ring_buff;

  auto written_bytes_numb =
      ring_buff.Write(src.begin(), src.begin() + str_len_without_null);
  ASSERT_EQ(str_len_without_null, written_bytes_numb);
  ASSERT_EQ(str_len_without_null, ring_buff.Size());

  std::array<char, array_size> dst_{0};
  auto read_bytes_numb = ring_buff.Read(dst_.begin(), dst_.size());
  ASSERT_EQ(written_bytes_numb, read_bytes_numb);

  ASSERT_EQ(
      0, memcmp(
             static_cast<const void*>(src.data()),
             static_cast<const void*>(dst_.data()), str_len_without_null));
}

TEST(RingBuff, Clear) {
  std::string src{"Hello World!"};
  constexpr std::size_t ringbuff_size_in_bytes{100};
  paraos::RingBuff<char, ringbuff_size_in_bytes> ring_buff;

  auto written_bytes_numb =
      ring_buff.Write(static_cast<const void*>(src.data()), src.size());

  ASSERT_EQ(written_bytes_numb, ring_buff.Size());

  ring_buff.Clear();
  ASSERT_EQ(0U, ring_buff.Size());
}

TEST(RingBuff, Skip) {
  std::string src{"Hello World!"};
  constexpr std::size_t ringbuff_size_in_bytes{100};
  paraos::RingBuff<char, ringbuff_size_in_bytes> ring_buff;

  auto written_bytes_numb =
      ring_buff.Write(static_cast<const void*>(src.data()), src.size());

  ASSERT_EQ(written_bytes_numb, ring_buff.Size());

  constexpr std::size_t skip_need{10};
  auto skip_bytes = ring_buff.Skip(skip_need);
  ASSERT_EQ(skip_need, skip_bytes);

  ASSERT_EQ(written_bytes_numb - skip_need, ring_buff.Size());
}

TEST(RingBuff, WriteSpanThenReadSpan) {
  std::string src{"Hello World!"};
  constexpr std::size_t ringbuff_size_in_bytes{100};
  constexpr size_t array_size{128};

  paraos::RingBuff<char, ringbuff_size_in_bytes> ring_buff;

  auto written_bytes_numb = ring_buff.Write(src);
  ASSERT_EQ(src.size(), written_bytes_numb);
  ASSERT_EQ(src.size(), ring_buff.Size());

  std::array<char, array_size> dst_{0};
  auto read_bytes_numb = ring_buff.Read(dst_);
  ASSERT_EQ(written_bytes_numb, read_bytes_numb);

  ASSERT_EQ(
      0, memcmp(
             static_cast<const void*>(src.data()),
             static_cast<const void*>(dst_.data()), src.size()));

  // All data was read.
  ASSERT_EQ(ringbuff_size_in_bytes, ring_buff.Free());
}

TEST(RingBuff, WriteOverflow) {
  std::string src{"Hello World!"};
  constexpr std::size_t ringbuff_size_in_bytes{10};

  paraos::RingBuff<char, ringbuff_size_in_bytes> ring_buff;
  auto written_bytes_numb = ring_buff.Write(src);
  ASSERT_EQ(0U, written_bytes_numb);
}

TEST(RingBuff, WriteIteratorThenReadUint32) {
  constexpr std::size_t ringbuff_size_in_elements{32};
  constexpr std::size_t array_size{128};

  const std::vector<std::uint32_t> src{1U, 2U, 3U, 4U, 5U};
  const std::size_t expected_written_bytes = src.size() * sizeof(std::uint32_t);

  paraos::RingBuff<std::uint32_t, ringbuff_size_in_elements> ring_buff;

  const auto written_bytes_numb = ring_buff.Write(src.begin(), src.end());
  ASSERT_EQ(expected_written_bytes, written_bytes_numb);
  ASSERT_EQ(expected_written_bytes, ring_buff.Size());

  std::array<std::uint32_t, array_size> dst_{0};
  const auto read_bytes_numb =
      ring_buff.Read(dst_.data(), dst_.size() * sizeof(std::uint32_t));
  ASSERT_EQ(expected_written_bytes, read_bytes_numb);

  ASSERT_EQ(
      0, memcmp(
             static_cast<const void*>(src.data()),
             static_cast<const void*>(dst_.data()), expected_written_bytes));
}
