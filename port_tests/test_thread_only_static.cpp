/// @file test_thread_only_static.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <atomic>
#include <cstdlib>
#include <iostream>

#include "paraos_critical.hpp"
#include "paraos_thread.hpp"

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables,
// *-readability-identifier-naming)

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

constexpr std::size_t EXPECTED_THREADS{3};

inline void DefaultDelegate() { paraos::Thread::DelayMs(100); }
constexpr paraos::thread_delegate_type thread_default_delegate =
    etl::delegate<void()>::create<DefaultDelegate>();

paraos::Thread check_test_complete_and_exit{paraos::ThreadAttr{
    "Check test complete", paraos::GetStackMinimumSizeInBytes(),
    paraos::ThreadPriority::kRealTime, DeletedObjectsCnt,
    thread_default_delegate}};

void ExitFromTest() {
  if (cnt >= EXPECTED_THREADS) {
    check_test_complete_and_exit.Finished();
    constexpr std::size_t delay_ms{0};
    PrintDebug("Ready to exit, delay ms " << delay_ms, "ExitFromTest");

    check_test_complete_and_exit.Finished();

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

class MyThreadStatic {
 public:
  explicit MyThreadStatic(const paraos::ThreadAttr& attr) : thread_{attr} {
    thread_.RegisterDelegate(
        paraos::thread_delegate_type::create<
            MyThreadStatic, &MyThreadStatic::Processing>(*this));
  }

  virtual ~MyThreadStatic() = default;

  void Processing() {
    PrintDebug("Calling Processing()", thread_.GiveName());

    PrintDebug("Calling thread_.Finished()", thread_.GiveName());
    thread_.Finished();
    ++cnt;
  }

 private:
  paraos::Thread thread_;
};

auto main() -> int {
  {
    auto delegate = etl::delegate<void()>::create<ExitFromTest>();
    check_test_complete_and_exit.RegisterDelegate(delegate);
  }

  {
    paraos::ThreadAttr attr;
    attr.thread_name = "My thread static one";
    attr.dtor_callback = DeletedObjectsCnt;

    const static MyThreadStatic static_thread{attr};
  }

  {
    paraos::ThreadAttr attr;
    attr.thread_name = "My thread static two";
    attr.dtor_callback = DeletedObjectsCnt;
    const static MyThreadStatic static_thread{attr};
  }

  {
    paraos::ThreadAttr attr;
    attr.thread_name = "My thread static three";
    attr.dtor_callback = DeletedObjectsCnt;
    const static MyThreadStatic static_thread{attr};
  }

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();

  return 0;
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables,
// *-readability-identifier-naming)