/// @file test_jthread_basic.cpp
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
/// @brief Basic standalone test for paraos::jthread.

#include <atomic>
#include <cstdlib>
#include <iostream>

#include "paraos_jthread.hpp"
#include "paraos_thread.hpp"

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables,
// *-readability-identifier-naming)

namespace {
std::atomic<std::size_t> counter{0};
constexpr std::size_t kExpectedCounter{2};

void my_func(int a, float f) {
  counter.fetch_add(1);
  (void)a;
  (void)f;
}
}  // namespace

auto main() -> int {
  {
    std::size_t size{0};
    paraos::jthread thread([&size](const paraos::stop_token& token) {
      counter.fetch_add(1);
      while (!token.stop_requested()) {
        ++size;
        paraos::Thread::DelayMs(10);
      }
    });

    paraos::jthread thread2([](const paraos::stop_token& token) {
      my_func(42, 3.14f);
      while (!token.stop_requested()) {
        paraos::Thread::DelayMs(10);
      }
    });

#if defined(PARAOS_LIKE_FREERTOS)
    // The FreeRTOS POSIX port never returns from StartScheduler(). Run the
    // stop/join/check sequence from a second task and terminate the process
    // from inside the scheduler.
    paraos::jthread stopper([&thread, &thread2](paraos::stop_token /*token*/) {
      while (counter.load() < 2) {
        paraos::Thread::DelayMs(10);
      }

      (void)thread.request_stop();
      thread.join();
      (void)thread2.request_stop();
      thread2.join();

      std::_Exit(
          counter.load() == kExpectedCounter ? EXIT_SUCCESS : EXIT_FAILURE);
    });
#endif

    paraos::Thread::StartScheduler();

#if !defined(PARAOS_LIKE_FREERTOS)
    while (counter.load() < 2) {
      paraos::Thread::DelayMs(10);
    }

    (void)thread.request_stop();
    thread.join();
    (void)thread2.request_stop();
    thread2.join();
#endif
  }

#if !defined(PARAOS_LIKE_FREERTOS)
  if (counter.load() == kExpectedCounter) {
    std::cout << "OK\n";
  } else {
    std::cout << "FAIL: counter=" << counter.load() << "\n";
    return EXIT_FAILURE;
  }

  paraos::Thread::Exit();
#endif

  return 0;
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables,
// *-readability-identifier-naming)
