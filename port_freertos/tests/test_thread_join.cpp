#include <iostream>

#include "paraos_critical.hpp"
#include "paraos_thread.hpp"

static std::size_t cnt{0};

#if defined(__linux__)
#define configUSE_IDLE_HOOK 1
static bool threads_deleted_flag = false;
/// @brief The idle task runs at the very lowest priority, so such an idle hook
/// function will only get executed when there are no tasks of higher priority
/// that are able to run.
extern "C" void PARAOS_ATTR_WEAK vApplicationIdleHook(void) {
  std::cout << uxTaskGetNumberOfTasks() << std::endl;
  if (cnt == 4 && threads_deleted_flag == true) {
    std::cout << "Exiting program..." << std::endl;
    _Exit(0);
  }
  if (cnt == 3) {
    std::cout << "Deleting all threads..." << std::endl;
    paraos::Thread::DeleteAll();
    threads_deleted_flag = true;
    cnt++;
  }
}
#endif

struct TestMessage : public paraos::Thread {
  TestMessage(const std::string name = "default thread name")
      : paraos::Thread{name, 3072ull, paraos::ThreadPriority::kNormal, true} {
    Start();
  }
  void Run() override {
    const paraos::CriticalSection critical;
    std::cout << Name() << " RTOS thread Cnt is " << cnt << std::endl;
    ++cnt;
  }

 private:
};

int main() {
  TestMessage print1{"Thread 1"};
  TestMessage print2{"Thread 2"};
  TestMessage print3{"Thread 3"};
  paraos::Thread::StartScheduler();

  return 0;
}
