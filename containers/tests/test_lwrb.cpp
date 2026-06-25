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
    const paraos::ring_buff<char, 2> ring_buff;
  } catch (paraos::ring_buff_ctor_error_exception& e) {
    FAIL() << "paraos::ring_buff can't throw exception" << "\n";
  }
}

TEST(RingBuff, Capacity) {
  const paraos::ring_buff<char, 2> ring_buff;
  ASSERT_EQ(2, ring_buff.capacity());
  ASSERT_EQ(2, ring_buff.free_space());
}

TEST(RingBuff, IsEmpty) {
  paraos::ring_buff<char, 2> ring_buff;
  ASSERT_TRUE(ring_buff.is_empty());

  const char symb{'h'};
  ASSERT_EQ(sizeof(symb), ring_buff.write(&symb, sizeof(symb)));
  ASSERT_FALSE(ring_buff.is_empty());

  ring_buff.clear();
  ASSERT_TRUE(ring_buff.is_empty());
}

TEST(RingBuff, IsFull) {
  paraos::ring_buff<char, 2> ring_buff;
  const char symb{'h'};

  ASSERT_EQ(sizeof(symb), ring_buff.write(&symb, sizeof(symb)));
  ASSERT_FALSE(ring_buff.is_full());

  ASSERT_EQ(sizeof(symb), ring_buff.write(&symb, sizeof(symb)));
  ASSERT_TRUE(ring_buff.is_full());
}

TEST(RingBuff, WriteThenRead) {
  std::string src{"Hello World!"};
  constexpr std::size_t ringbuff_size_in_bytes{100};
  constexpr size_t array_size{128};

  paraos::ring_buff<char, ringbuff_size_in_bytes> ring_buff;

  ASSERT_EQ(ringbuff_size_in_bytes, ring_buff.free_space());

  auto written_bytes_numb =
      ring_buff.write(static_cast<const void*>(src.c_str()), src.size());
  ASSERT_EQ(src.size(), written_bytes_numb);
  ASSERT_EQ(src.size(), ring_buff.size());

  std::array<char, array_size> dst_{0};
  auto read_bytes_numb = ring_buff.read(dst_.data(), dst_.size());
  ASSERT_EQ(written_bytes_numb, read_bytes_numb);

  ASSERT_EQ(
      0, memcmp(
             static_cast<const void*>(src.data()),
             static_cast<const void*>(dst_.data()), src.size()));

  // All date read.
  ASSERT_EQ(ringbuff_size_in_bytes, ring_buff.free_space());
}

TEST(RingBuff, WriteThenReadIterator) {
  constexpr std::size_t ringbuff_size_in_bytes{100};
  constexpr size_t array_size{128};

  // User std::array instead std::stirng or check simple iterator.
  constexpr std::array<char, ringbuff_size_in_bytes> src{"Hello World!"};
  const auto str_len_without_null = strlen(src.data());

  paraos::ring_buff<char, ringbuff_size_in_bytes> ring_buff;

  auto written_bytes_numb =
      ring_buff.write(src.begin(), src.begin() + str_len_without_null);
  ASSERT_EQ(str_len_without_null, written_bytes_numb);
  ASSERT_EQ(str_len_without_null, ring_buff.size());

  std::array<char, array_size> dst_{0};
  auto read_bytes_numb = ring_buff.read(dst_.begin(), dst_.size());
  ASSERT_EQ(written_bytes_numb, read_bytes_numb);

  ASSERT_EQ(
      0, memcmp(
             static_cast<const void*>(src.data()),
             static_cast<const void*>(dst_.data()), str_len_without_null));
}

TEST(RingBuff, Clear) {
  std::string src{"Hello World!"};
  constexpr std::size_t ringbuff_size_in_bytes{100};
  paraos::ring_buff<char, ringbuff_size_in_bytes> ring_buff;

  auto written_bytes_numb =
      ring_buff.write(static_cast<const void*>(src.data()), src.size());

  ASSERT_EQ(written_bytes_numb, ring_buff.size());

  ring_buff.clear();
  ASSERT_EQ(0U, ring_buff.size());
}

TEST(RingBuff, Skip) {
  std::string src{"Hello World!"};
  constexpr std::size_t ringbuff_size_in_bytes{100};
  paraos::ring_buff<char, ringbuff_size_in_bytes> ring_buff;

  auto written_bytes_numb =
      ring_buff.write(static_cast<const void*>(src.data()), src.size());

  ASSERT_EQ(written_bytes_numb, ring_buff.size());

  constexpr std::size_t skip_need{10};
  auto skip_bytes = ring_buff.skip(skip_need);
  ASSERT_EQ(skip_need, skip_bytes);

  ASSERT_EQ(written_bytes_numb - skip_need, ring_buff.size());
}

TEST(RingBuff, WriteSpanThenReadSpan) {
  std::string src{"Hello World!"};
  constexpr std::size_t ringbuff_size_in_bytes{100};
  constexpr size_t array_size{128};

  paraos::ring_buff<char, ringbuff_size_in_bytes> ring_buff;

  auto written_bytes_numb = ring_buff.write(src);
  ASSERT_EQ(src.size(), written_bytes_numb);
  ASSERT_EQ(src.size(), ring_buff.size());

  std::array<char, array_size> dst_{0};
  auto read_bytes_numb = ring_buff.read(dst_);
  ASSERT_EQ(written_bytes_numb, read_bytes_numb);

  ASSERT_EQ(
      0, memcmp(
             static_cast<const void*>(src.data()),
             static_cast<const void*>(dst_.data()), src.size()));

  // All data was read.
  ASSERT_EQ(ringbuff_size_in_bytes, ring_buff.free_space());
}

TEST(RingBuff, WriteOverflow) {
  std::string src{"Hello World!"};
  constexpr std::size_t ringbuff_size_in_bytes{10};

  paraos::ring_buff<char, ringbuff_size_in_bytes> ring_buff;
  auto written_bytes_numb = ring_buff.write(src);
  ASSERT_EQ(0U, written_bytes_numb);
}

TEST(RingBuff, WriteIteratorThenReadUint32) {
  constexpr std::size_t ringbuff_size_in_elements{32};
  constexpr std::size_t array_size{128};

  const std::vector<std::uint32_t> src{1U, 2U, 3U, 4U, 5U};
  const std::size_t expected_written_bytes = src.size() * sizeof(std::uint32_t);

  paraos::ring_buff<std::uint32_t, ringbuff_size_in_elements> ring_buff;

  const auto written_bytes_numb = ring_buff.write(src.begin(), src.end());
  ASSERT_EQ(expected_written_bytes, written_bytes_numb);
  ASSERT_EQ(expected_written_bytes, ring_buff.size());

  std::array<std::uint32_t, array_size> dst_{0};
  const auto read_bytes_numb =
      ring_buff.read(dst_.data(), dst_.size() * sizeof(std::uint32_t));
  ASSERT_EQ(expected_written_bytes, read_bytes_numb);

  ASSERT_EQ(
      0, memcmp(
             static_cast<const void*>(src.data()),
             static_cast<const void*>(dst_.data()), expected_written_bytes));
}
