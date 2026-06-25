/// @file test_timer.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// @brief Standalone skeleton test for paraos::timer.

#include <cstddef>

#include "paraos_timer.hpp"

namespace {

constexpr std::size_t k_period_ms{100};

class test_timer_app : public paraos::timer {
 public:
  test_timer_app() : paraos::timer(k_period_ms, false, true, "test_timer") {}

  void run() override {}
};

}  // namespace

auto main() -> int {
  test_timer_app timer;
  timer.start();
  timer.stop();
  return 0;
}
