/// @file test_paraos_thread_only_stack.cpp
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

#define PrintDebug(__message__, __object_name__)                             \
  {                                                                          \
    const paraos::CriticalSection macro_critical;                            \
    std::cout << "DM: '" << __object_name__ << "': " << __message__ << "\n"; \
  }

// clang-format off
// NOLINTBEGIN(misc-include-cleaner, readability-magic-numbers, hicpp-special-member-functions, misc-const-correctness)
// clang-format on
#include <iostream>
#include <string>

#include "paraos_base.hpp"
#include "paraos_critical.hpp"
#include "paraos_thread.hpp"
#include "paraos_utils.hpp"

namespace {
std::atomic<std::size_t> cnt{0};

constexpr std::size_t expected_threads{3};

std::atomic<std::size_t> deleted_objects_cnt;

void DeletedObjectsCnt() {
  const paraos::CriticalSection critical;
  ++deleted_objects_cnt;

  PrintDebug(
      "Deleted objects cnt is " << deleted_objects_cnt, "DeletedObjectsCnt");
}

inline void DefaultDelegate() { paraos::Thread::DelayMs(100); }
constexpr paraos::thread_delegate_type thread_default_delegate =
    etl::delegate<void()>::create<DefaultDelegate>();

paraos::Thread check_test_complete_and_exit{paraos::ThreadAttr{
    "Check test complete", paraos::GetStackMinimumSizeInBytes(),
    paraos::ThreadPriority::kRealTime, DeletedObjectsCnt,
    thread_default_delegate}};

void ExitFromTest() {
  if (deleted_objects_cnt >= expected_threads) {
    check_test_complete_and_exit.Finished();
    constexpr paraos::delay_type delay_ms{0};
    PrintDebug("Ready to exit, delay ms " << delay_ms, "ExitFromTest");
    paraos::Thread::DelayMs(delay_ms);

    PrintDebug("Call paraos::Thread::Exit();", "ExitFromTest");
    paraos::Thread::Exit();
  }

  PrintDebug("Yeld resources", "ExitFromTest");
  paraos::Thread::DelayMs(10);
}
}  // namespace

class MyThreadDynamic : public paraos::Base {
 public:
  explicit MyThreadDynamic(const paraos::ThreadAttr &attr) : thread_{attr} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<
            MyThreadDynamic, &MyThreadDynamic::Processing>(*this));

    // Check priority API. For test only. In real application, ctor of
    // paraos::Thread set priority from attr.
    thread_.SetPriority(attr.priority);
    auto priority = thread_.GetPriority();

#if !defined(PARAOS_LIKE_UNIX)
    // Can't change priority without root privileges on UNIX.
    PARAOS_CHECK_ASSERT(attr.priority == priority);
#endif
    PARAOS_ATTR_UNUSED_VAR(priority);
  }

  ~MyThreadDynamic() override {
    PrintDebug("~MyThreadDynamic", thread_.GiveName());
  };

 private:
  void Processing() {
    PrintDebug("Calls Processing()", thread_.GiveName());

    ++cnt;

    // Break while cycle.
    thread_.Finished(this);
  }

  paraos::Thread thread_;
};

auto main() -> int {
  {
    auto delegate = etl::delegate<void()>::create<ExitFromTest>();
    check_test_complete_and_exit.RegisterDelegate(delegate);
  }

  try {
    // no safe pointer because MyThreadDynamic{} delete self after all
    // computing will be complete.
    for (std::size_t i = 0; i < expected_threads; ++i) {
      std::string name{"My thread dynamic " + std::to_string(i)};
      paraos::ThreadAttr attr;
      attr.thread_name = name;
      attr.dtor_callback = DeletedObjectsCnt;

      // No need put address in the pointer because MyThreadDynamic instance
      // delete self when the computing will be complete.
      new MyThreadDynamic(attr);
    }
  } catch (const paraos::thread_exception &e) {
    PrintDebug(e.what(), "main()");
  } catch (const std::exception &e) {
    PrintDebug(e.what(), "main()");
  }

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  return 0;
  // clang-format off
}

// NOLINTEND(misc-include-cleaner, readability-magic-numbers, hicpp-special-member-functions, misc-const-correctness)
// clang-format on
