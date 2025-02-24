/// @file test_paraos_thread_only_global.cpp
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

// clang-format off
// NOLINTBEGIN(misc-include-cleaner, readability-magic-numbers, hicpp-special-member-functions, misc-const-correctness)
// clang-format on
#include <cstdlib>
#include <iostream>

#include "paraos_critical.hpp"
#include "paraos_thread.hpp"

#define PrintDebug(__message__, __object_name__)                             \
  {                                                                          \
    const paraos::CriticalSection macro_critical;                            \
    std::cout << "DM: '" << __object_name__ << "': " << __message__ << "\n"; \
  }

namespace {
std::atomic<std::size_t> deleted_objects_cnt;

void DeletedObjectsCnt() {
  ++deleted_objects_cnt;

  PrintDebug(
      "Deleted objects cnt is " << deleted_objects_cnt, "DeletedObjectsCnt");
}

std::atomic<std::size_t> cnt{0};
constexpr std::size_t expected_threads{3};

inline void DefaultDelegate() { paraos::Thread::DelayMs(100); }
constexpr paraos::thread_delegate_type thread_default_delegate =
    etl::delegate<void()>::create<DefaultDelegate>();

paraos::Thread check_test_complete_and_exit{paraos::ThreadAttr{
    "Check test complete", paraos::GetStackMinimumSizeInBytes(),
    paraos::ThreadPriority::kRealTime, DeletedObjectsCnt,
    thread_default_delegate}};

paraos::Thread my_thread_global_one{paraos::ThreadAttr{
    "Global thread one", paraos::GetStackMinimumSizeInBytes(),
    paraos::ThreadPriority::kLowest, DeletedObjectsCnt,
    thread_default_delegate}};

paraos::Thread my_thread_global_two{paraos::ThreadAttr{
    "Global thread two", paraos::GetStackMinimumSizeInBytes(),
    paraos::ThreadPriority::kNormal, DeletedObjectsCnt,
    thread_default_delegate}};

paraos::Thread my_thread_global_three{paraos::ThreadAttr{
    "Global thread three", paraos::GetStackMinimumSizeInBytes(),
    paraos::ThreadPriority::kRealTime, DeletedObjectsCnt,
    thread_default_delegate}};

void ExitFromTest() {
  if (cnt >= expected_threads) {
    check_test_complete_and_exit.Finished();
    constexpr std::size_t delay_ms{0};
    PrintDebug("Ready to exit, delay ms " << delay_ms, "ExitFromTest");
    paraos::Thread::DelayMs(delay_ms);

    PrintDebug("Call paraos::Thread::Exit();", "ExitFromTest");
#if defined(PARAOS_LIKE_FREERTOS)
    // Forces program exit to reduce execution time. Needed to terminate tests
    // early, especially when running multiple tests. In other case, program
    // will exit in 1 second later.
    std::_Exit(EXIT_SUCCESS);
#else
    paraos::Thread::Exit();
#endif
  }

  PrintDebug("Yeld resources", "ExitFromTest");
  paraos::Thread::DelayMs(10);
}

void ProcessingOne() {
  PrintDebug(
      "ProcessingOne() calling Finished()", my_thread_global_one.GiveName());
  my_thread_global_one.Finished();
  ++cnt;
}

void ProcessingTwo() {
  PrintDebug(
      "ProcessingTwo calling Finished()", my_thread_global_two.GiveName());
  my_thread_global_two.Finished();
  ++cnt;
}

void ProcessingThree() {
  PrintDebug(
      "ProcessingThree() calling Finished()",
      my_thread_global_three.GiveName());
  my_thread_global_three.Finished();
  ++cnt;
}
}  // namespace

auto main() -> int {
  {
    auto delegate = etl::delegate<void()>::create<ExitFromTest>();
    check_test_complete_and_exit.RegisterDelegate(delegate);
  }

  {
    auto delegate = etl::delegate<void()>::create<ProcessingOne>();
    my_thread_global_one.RegisterDelegate(delegate);
  }

  {
    auto delegate = etl::delegate<void()>::create<ProcessingTwo>();
    my_thread_global_two.RegisterDelegate(delegate);
  }

  {
    auto delegate = etl::delegate<void()>::create<ProcessingThree>();
    my_thread_global_three.RegisterDelegate(delegate);
  }

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  return 0;
  // clang-format off
}

// NOLINTEND(misc-include-cleaner, readability-magic-numbers, hicpp-special-member-functions, misc-const-correctness)
// clang-format on
