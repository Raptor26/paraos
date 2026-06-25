/// @file test_runtime_profiler.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include "paraos_runtime_profiler.hpp"

namespace {
uint16_t high;
uint16_t low;

class Profiler : public testing::Test {
 public:
  struct LowCnt {
    auto operator()() volatile -> decltype(&low) { return &low; }
  };

  struct HightCnt {
    auto operator()() volatile -> decltype(&high) { return &high; }
  };

 protected:
  void SetUp() override {
    high = 0U;
    low = 0U;
  }
};
}  // namespace

TEST_F(Profiler, TimerProfiler) {
  using embedded_timer_t = paraos::embedded_timer<LowCnt, HightCnt>;

  const embedded_timer_t embedded_timer;
  paraos::timer_profiler profiler{embedded_timer};
  EXPECT_EQ(0U, profiler.last_duration());
}

TEST_F(Profiler, LongCnt) {
  constexpr std::uint16_t increment{2};
  paraos::embedded_profiler<LowCnt, HightCnt> profiler;

  profiler.start();
  low += increment;
  EXPECT_EQ(increment, profiler.stop());
}

TEST_F(Profiler, ShortCnt) {
  constexpr std::uint16_t increment{2};
  paraos::embedded_profiler<LowCnt, paraos::high_count_default> profiler;
  paraos::embedded_profiler<LowCnt> profiler_high_default;

  profiler.start();
  profiler_high_default.start();
  low += increment;
  EXPECT_EQ(increment, profiler.stop());
  EXPECT_EQ(increment, profiler_high_default.stop());
}

TEST_F(Profiler, ShortCntOneOverflow) {
  low = std::numeric_limits<std::uint16_t>::max();
  constexpr std::uint16_t increment{3};
  paraos::embedded_profiler<LowCnt> profiler;

  profiler.start();
  low += increment;
  EXPECT_EQ(increment, profiler.stop());
  EXPECT_EQ(increment, profiler.last_duration());
}

TEST_F(Profiler, ShortCntTwoOverflow) {
  low = std::numeric_limits<std::uint16_t>::max();
  constexpr std::uint16_t first_increment{3};
  paraos::embedded_profiler<LowCnt> profiler;

  profiler.start();
  low += first_increment;
  EXPECT_EQ(
      static_cast<decltype(profiler.stop())>(first_increment), profiler.stop());

  constexpr std::uint16_t second_increment{
      std::numeric_limits<std::uint16_t>::max()};
  low += second_increment;
  EXPECT_EQ(
      static_cast<decltype(profiler.stop())>(
          first_increment + second_increment),
      profiler.stop());
}

TEST_F(Profiler, LongCntOneOverflow) {
  low = std::numeric_limits<std::uint16_t>::max();
  high = std::numeric_limits<std::uint16_t>::max();
  constexpr std::uint16_t increment{3};
  paraos::embedded_profiler<LowCnt, HightCnt> profiler;

  profiler.start();
  low += increment;
  high = 0U;
  EXPECT_EQ(increment, profiler.stop());
}

TEST_F(Profiler, RAII) {
  constexpr std::uint16_t increment{3};
  paraos::embedded_profiler<LowCnt, HightCnt> profiler;
  {
    const paraos::profiler_raii profiler_raii(profiler);

    low += increment;

    // ... some long code block

    // Dtor of profiler_raii stop the timer.
  }

  EXPECT_EQ(increment, profiler.last_duration());
}

TEST_F(Profiler, RAIIPeriod) {
  constexpr std::uint16_t increment{3};
  paraos::embedded_profiler<LowCnt, HightCnt> profiler;
  {
    /// now profiler will indicates period between calling code below.
    const paraos::profiler_period_raii period_calling(profiler);

    low += increment;

    // ... some long code block
  }
}
