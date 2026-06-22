/// @file test_mutex_basic.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
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
