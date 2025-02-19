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

#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>

#include "paraos_base.hpp"
#include "paraos_critical.hpp"
#include "paraos_thread_v2.hpp"
#include "paraos_timer.hpp"
#include "paraos_trace.hpp"
#include "paraos_utils.hpp"

std::atomic<std::size_t> cnt{0};
constexpr std::size_t expected_threads{400};

std::atomic<std::size_t> deleted_objects_cnt;

void DeletedObjectsCnt() {
  ++deleted_objects_cnt;

  PrintDebug(
      "Deleted objects cnt is " << deleted_objects_cnt, "DeletedObjectsCnt");
}

inline void DefaultDelegate() { paraos::v2::Thread::DelayMs(100); }
constexpr paraos::v2::thread_delegate_type thread_default_delegate =
    etl::delegate<void()>::create<DefaultDelegate>();

paraos::v2::Thread check_test_complete_and_exit{paraos::v2::ThreadAttr{
    "Check test complete", paraos::GetStackMinimumSizeInBytes(),
    paraos::v2::ThreadPriority::kRealTime, DeletedObjectsCnt,
    thread_default_delegate}};

void ExitFromTest() {
  // Mult 2 because each MyThreadDynamic instance creates one additional dynamic
  // thread.
  constexpr auto expected_delete_threads_numb{expected_threads * 2};
  if (deleted_objects_cnt >= expected_delete_threads_numb) {
    constexpr std::size_t delay_ms{0};

    PrintDebug("Ready to exit, delay ms " << delay_ms, "ExitFromTest");
    paraos::v2::Thread::DelayMs(delay_ms);

    PrintDebug("Call paraos::v2::Thread::Exit();", "ExitFromTest");

    paraos::v2::Thread::Exit();
    check_test_complete_and_exit.Finished();
  }

  PrintDebug("Yeld resources", "ExitFromTest");
  paraos::v2::Thread::DelayMs(1000);
}

class MyThreadDynamic : public paraos::Base {
 public:
  explicit MyThreadDynamic(const paraos::v2::ThreadAttr &attr)
      : thread_one_{attr}, thread_two_{attr} {
    thread_one_.RegisterDelegate(
        paraos::v2::thread_delegate_type::create<
            MyThreadDynamic, &MyThreadDynamic::ProcessingOne>(*this));

    // -------------------------------------------------------------------------

    thread_two_.RegisterDelegate(
        paraos::v2::thread_delegate_type::create<
            MyThreadDynamic, &MyThreadDynamic::ProcessingTwo>(*this));

    // -------------------------------------------------------------------------

    assert(
        (thread_dynamic_ == nullptr) &&
        "thread_dynamic_ must be nullptr when the ctor calls");

    std::string name{attr.thread_name};
    name += " Dynamic thread";
    paraos::v2::ThreadAttr thread_attr;
    thread_attr.thread_name = name;

    // Register delegate befor calls ctor.
    thread_attr.run_ = paraos::v2::thread_delegate_type::create<
        MyThreadDynamic, &MyThreadDynamic::ProcessingDynamic>(*this);

    thread_dynamic_ = new paraos::v2::Thread(thread_attr);
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
  paraos::v2::Thread thread_one_;

  /// @brief Free resources.
  paraos::v2::Thread thread_two_;

  /// @brief free only self in ProcessingDynamic().
  paraos::v2::Thread *thread_dynamic_{nullptr};

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
    for (std::size_t i = 0; i < expected_threads; ++i) {
      std::string name{"My thread for stack " + std::to_string(i)};
      paraos::v2::ThreadAttr attr;
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

  paraos::v2::Thread::StartScheduler();
  paraos::v2::Thread::DeleteAll();

  return 0;
}