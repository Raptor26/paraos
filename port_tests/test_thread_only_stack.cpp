/// @file test_thread_only_stack.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#define PrintDebug(__message__, __object_name__)                             \
  {                                                                          \
    const paraos::CriticalSection macro_critical;                            \
    std::cout << "DM: '" << __object_name__ << "': " << __message__ << "\n"; \
  }

#include <cstdlib>
#include <iostream>
#include <string>

#include "paraos_base.hpp"
#include "paraos_critical.hpp"
#include "paraos_thread.hpp"
#include "paraos_utils.hpp"

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables,
// *-readability-identifier-naming)

namespace {
std::atomic<std::size_t> cnt{0};

constexpr std::size_t EXPECTED_THREADS{3};

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
  if (deleted_objects_cnt >= EXPECTED_THREADS) {
    check_test_complete_and_exit.Finished();
    constexpr paraos::delay_type delay_ms{0};
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
    for (std::size_t i = 0; i < EXPECTED_THREADS; ++i) {
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
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables,
// *-readability-identifier-naming)