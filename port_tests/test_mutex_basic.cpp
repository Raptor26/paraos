/// @file test_mutex_basic.cpp
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
/// @brief Basic standalone test for paraos::mutex.

#include <cstdlib>

#include "paraos_mutex_std.hpp"

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables,
// *-readability-identifier-naming)

#if !defined(PARAOS_LIKE_FREERTOS)
#include <atomic>
#include <iostream>
#include <mutex>

namespace {
std::atomic<bool> is_test_passed{false};

auto RunTests() -> bool {
  paraos::mutex test_mutex;

  {
    std::lock_guard<paraos::mutex> lock(test_mutex);
  }

  {
    std::unique_lock<paraos::mutex> lock(test_mutex);
    if (!lock.owns_lock()) {
      return false;
    }
    lock.unlock();
    if (lock.owns_lock()) {
      return false;
    }
  }

  if (test_mutex.try_lock()) {
    test_mutex.unlock();
  } else {
    return false;
  }

  test_mutex.lock();
  test_mutex.unlock();

  return true;
}
}  // namespace
#endif

auto main() -> int {
#if defined(PARAOS_LIKE_FREERTOS)
  // The FreeRTOS POSIX port on macOS cannot start the scheduler reliably in
  // this environment (test_jthread_basic has the same limitation). Verify that
  // the mutex can be constructed and destroyed from the main context before
  // the scheduler starts, which still exercises xSemaphoreCreateMutex /
  // vSemaphoreDelete.
  {
    paraos::mutex test_mutex;
    (void)test_mutex;
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
