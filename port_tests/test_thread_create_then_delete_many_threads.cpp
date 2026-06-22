/// @file test_thread_create_then_delete_many_threads.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "paraos_base.hpp"
#include "paraos_critical.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_thread.hpp"
#include "paraos_timer.hpp"
#include "paraos_trace.hpp"
#include "paraos_utils.hpp"

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables,
// *-readability-identifier-naming)

#define PrintDebug(__message__, __object_name__)                             \
  {                                                                          \
    const paraos::CriticalSection macro_critical;                            \
    std::cout << "DM: '" << __object_name__ << "': " << __message__ << "\n"; \
  }

std::atomic<std::size_t> cnt{0};
constexpr std::size_t EXPECTED_THREADS{400};

std::atomic<std::size_t> deleted_objects_cnt;

void DeletedObjectsCnt() {
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
  // Mult 2 because each MyThreadDynamic instance creates one additional dynamic
  // thread.
  constexpr auto expected_delete_threads_numb{EXPECTED_THREADS * 2};
  if (deleted_objects_cnt >= expected_delete_threads_numb) {
    check_test_complete_and_exit.Finished();
    constexpr std::size_t DELAY_MS{0};
    PrintDebug("Ready to exit, delay ms " << DELAY_MS, "ExitFromTest");
    paraos::Thread::DelayMs(DELAY_MS);
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
  paraos::Thread::DelayMs(1000);
}

class MyThreadDynamic : public paraos::Base {
 public:
  explicit MyThreadDynamic(const paraos::ThreadAttr &attr)
      : thread_one_{attr}, thread_two_{attr} {
    thread_one_.RegisterDelegate(
        paraos::thread_delegate_type::create<
            MyThreadDynamic, &MyThreadDynamic::ProcessingOne>(*this));

    // -------------------------------------------------------------------------

    thread_two_.RegisterDelegate(
        paraos::thread_delegate_type::create<
            MyThreadDynamic, &MyThreadDynamic::ProcessingTwo>(*this));

    // -------------------------------------------------------------------------

    assert(
        (thread_dynamic_ == nullptr) &&
        "thread_dynamic_ must be nullptr when the ctor calls");

    std::string name{attr.thread_name};
    name += " Dynamic thread";
    paraos::ThreadAttr thread_attr;
    thread_attr.thread_name = name;

    // Register delegate befor calls ctor.
    thread_attr.run_ = paraos::thread_delegate_type::create<
        MyThreadDynamic, &MyThreadDynamic::ProcessingDynamic>(*this);

    thread_dynamic_ = new paraos::Thread(thread_attr);
    PrintDebug("Create thread on heap", thread_dynamic_->GiveName());
  }

  ~MyThreadDynamic() override {
    PrintDebug("~MyThreadDynamic", thread_one_.GiveName());
  };

 private:
  void ProcessingOne() {
    PrintDebug("ProcessingOne() calls", thread_one_.GiveName());
    thread_one_.Finished();

    PrintDebug("Give sem", thread_one_.GiveName());
    is_thread_one_complete_work_.Give();
  }

  void ProcessingTwo() {
    PrintDebug("ProcessingTwo() wait sems", thread_one_.GiveName());

    is_thread_one_complete_work_.Take();
    is_dynamic_thread_complete_work_.Take();
    PrintDebug("ProcessingTwo() calls Finished(this)", thread_one_.GiveName());
    thread_two_.Finished(this);
  }

  void ProcessingDynamic() {
    if (thread_dynamic_ != nullptr) {
      PrintDebug("ProcessingDynamic is finish", thread_dynamic_->GiveName());

      // Resources will be free later in thread handler.
      thread_dynamic_->Finished(thread_dynamic_);

      // Now we can reset pointer. It's mean we can use thread_dynamic_ again
      // for create a new thread from heap.
      thread_dynamic_ = nullptr;

      // PrintDebug("Give sem", thread_dynamic_->GiveName());
      is_dynamic_thread_complete_work_.Give();
    }
  }

  /// @brief Not free resources.
  paraos::Thread thread_one_;

  /// @brief Free resources.
  paraos::Thread thread_two_;

  /// @brief free only self in ProcessingDynamic().
  paraos::Thread *thread_dynamic_{nullptr};

  /// @brief Wait while thread 1 complete his work, only after that thread_two_
  /// can free all resources.
  paraos::SemaphoreBinary is_thread_one_complete_work_;

  /// @brief Wait while dynamic thread complete his work, only after that
  /// thread_two_ can free all resources.
  paraos::SemaphoreBinary is_dynamic_thread_complete_work_;
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
      const std::string name{"My thread for stack " + std::to_string(i)};
      paraos::ThreadAttr attr;
      attr.thread_name = name;
      attr.dtor_callback = DeletedObjectsCnt;

      // No need put address in the pointer because MyThreadDynamic instance
      // delete self when the computing will be complete (see ProcessingOne()).
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