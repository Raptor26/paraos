/// @file test_runtime_profiler.cpp
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

#include <cstdint>
#include <limits>

#include "paraos_runtime_profiler.hpp"

namespace {
uint16_t high;
uint16_t low;
}  // namespace

class Profiler : public testing::Test {
 public:
  struct LowCnt {
    auto operator()() volatile -> decltype(&low) { return &low; }
  };

  struct HightCnt {
    auto operator()() volatile -> decltype(&high) { return &high; }
  };

  void SetUp() override {
    high = 0U;
    low = 0U;
  }
};

TEST_F(Profiler, TimerProfiler) {
  using embedded_timer_t = paraos::EmbeddedTimer<LowCnt, HightCnt>;

  const embedded_timer_t embedded_timer;
  paraos::TimerProfiler profiler{embedded_timer};
  EXPECT_EQ(0U, profiler.LastDuration());
}

TEST_F(Profiler, LongCnt) {
  constexpr std::uint16_t increment{2};
  paraos::EmbeddedProfiler<LowCnt, HightCnt> profiler;

  profiler.Start();
  low += increment;
  EXPECT_EQ(increment, profiler.Stop());
}

TEST_F(Profiler, ShortCnt) {
  constexpr std::uint16_t increment{2};
  paraos::EmbeddedProfiler<LowCnt, paraos::HightCntDefault> profiler;
  paraos::EmbeddedProfiler<LowCnt> profiler_high_default;

  profiler.Start();
  profiler_high_default.Start();
  low += increment;
  EXPECT_EQ(increment, profiler.Stop());
  EXPECT_EQ(increment, profiler_high_default.Stop());
}

TEST_F(Profiler, ShortCntOneOverflow) {
  low = std::numeric_limits<std::uint16_t>::max();
  constexpr std::uint16_t increment{3};
  paraos::EmbeddedProfiler<LowCnt> profiler;

  profiler.Start();
  low += increment;
  EXPECT_EQ(increment, profiler.Stop());
  EXPECT_EQ(increment, profiler.LastDuration());
}

TEST_F(Profiler, ShortCntTwoOverflow) {
  low = std::numeric_limits<std::uint16_t>::max();
  constexpr std::uint16_t first_increment{3};
  paraos::EmbeddedProfiler<LowCnt> profiler;

  profiler.Start();
  low += first_increment;
  EXPECT_EQ(
      static_cast<decltype(profiler.Stop())>(first_increment), profiler.Stop());

  constexpr std::uint16_t second_increment{
      std::numeric_limits<std::uint16_t>::max()};
  low += second_increment;
  EXPECT_EQ(
      static_cast<decltype(profiler.Stop())>(
          first_increment + second_increment),
      profiler.Stop());
}

TEST_F(Profiler, LongCntOneOverflow) {
  low = std::numeric_limits<std::uint16_t>::max();
  high = std::numeric_limits<std::uint16_t>::max();
  constexpr std::uint16_t increment{3};
  paraos::EmbeddedProfiler<LowCnt, HightCnt> profiler;

  profiler.Start();
  low += increment;
  high = 0U;
  EXPECT_EQ(increment, profiler.Stop());
}

TEST_F(Profiler, RAII) {
  constexpr std::uint16_t increment{3};
  paraos::EmbeddedProfiler<LowCnt, HightCnt> profiler;
  {
    const paraos::ProfilerRAII profiler_raii(profiler);

    low += increment;

    // ... some long code block

    // Dtor of profiler_raii stop the timer.
  }

  EXPECT_EQ(increment, profiler.LastDuration());
}

TEST_F(Profiler, RAIIPeriod) {
  constexpr std::uint16_t increment{3};
  paraos::EmbeddedProfiler<LowCnt, HightCnt> profiler;
  {
    /// now profiler will indicates period between calling code below.
    const paraos::ProfilerPeriodRAII period_calling(profiler);

    low += increment;

    // ... some long code block
  }
}