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

#include "paraos_runtime_profiler.hpp"

using namespace paraos;

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
    high = 0u;
    low = 0u;
  }
};

TEST_F(Profiler, TimerProfiler) {
  using embedded_timer_t = EmbeddedTimer<LowCnt, HightCnt>;

  embedded_timer_t embedded_timer;
  TimerProfiler profiler{embedded_timer};
  EXPECT_EQ(0u, profiler.LastDuration());
}