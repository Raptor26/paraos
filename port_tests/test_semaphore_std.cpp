/// @file test_semaphore_std.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2025 Stilsoft
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
///
/// @brief Basic standalone test for paraos::counting_semaphore and
///        paraos::binary_semaphore.

#include <cstdlib>

#include "paraos_semaphore_std.hpp"

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables,
// *-readability-identifier-naming)

#if !defined(PARAOS_LIKE_FREERTOS)
#include <atomic>
#include <chrono>
#include <iostream>

namespace {
std::atomic<bool> is_test_passed{false};

auto RunTests() -> bool {
  paraos::counting_semaphore<2> sem_counting(0);
  paraos::binary_semaphore sem_binary(0);

  // counting_semaphore basic acquire/release
  sem_counting.release();
  sem_counting.acquire();
  if (sem_counting.try_acquire()) {
    return false;
  }

  sem_counting.release(2);
  if (!sem_counting.try_acquire()) {
    return false;
  }
  sem_counting.acquire();

  // binary_semaphore basic
  sem_binary.release();
  sem_binary.acquire();
  if (sem_binary.try_acquire()) {
    return false;
  }

  // try_acquire_for timeout
  paraos::binary_semaphore sem_empty(0);
  const auto start = std::chrono::steady_clock::now();
  const bool timed_out =
      !sem_empty.try_acquire_for(std::chrono::milliseconds(50));
  const auto elapsed = std::chrono::steady_clock::now() - start;
  if (!timed_out) {
    return false;
  }
  if (elapsed < std::chrono::milliseconds(50)) {
    return false;
  }

  // try_acquire_for success
  paraos::binary_semaphore sem_full(1);
  if (!sem_full.try_acquire_for(std::chrono::milliseconds(50))) {
    return false;
  }

  // max()
  if (paraos::counting_semaphore<2>::max() < 2) {
    return false;
  }
  if (paraos::binary_semaphore::max() != 1) {
    return false;
  }

  return true;
}
}  // namespace
#endif

auto main() -> int {
#if defined(PARAOS_LIKE_FREERTOS)
  {
    paraos::counting_semaphore<2> sem_counting(0);
    paraos::binary_semaphore sem_binary(0);
    (void)sem_counting;
    (void)sem_binary;
  }
  return EXIT_SUCCESS;
#else
  if (RunTests()) {
    is_test_passed.store(true);
    std::cout << "OK\n";
    return EXIT_SUCCESS;
  }

  std::cout << "FAIL\n";
  return EXIT_FAILURE;
#endif
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables,
// *-readability-identifier-naming)
