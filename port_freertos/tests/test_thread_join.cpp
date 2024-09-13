#include <cstdlib>
#include <iostream>

#include "paraos_critical.hpp"
#include "paraos_freertos_hooks.hpp"
#include "paraos_thread.hpp"

static std::size_t cnt{0};

static bool threads_deleted_flag = false;
void ExitAfterTestComplete() {
  if (cnt == 4 && threads_deleted_flag == true) {
    std::cout << "Exiting program..." << std::endl;
    vPortEndScheduler();
    // exit(EXIT_SUCCESS);
    // WM_CLOSE();
    // PostQuitMessage(EXIT_SUCCESS);
    // ::SendMessage(nullptr, WM_CLOSE, NULL, NULL);
  }

  if (cnt == 3) {
    std::cout << "Deleting all threads..." << std::endl;
    paraos::Thread::DeleteAll();
    threads_deleted_flag = true;
    cnt++;
  }

  // yeld processor resources.
  //   vTaskDelay(0);
}

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
};

int main() {
#if (configUSE_IDLE_HOOK == 1)
  // ExitAfterTestComplete will be called by scheduler in idle task after no
  // user task ready for execute.
  paraos::freertos_idle_fnc_ptr = ExitAfterTestComplete;
#endif

  TestMessage print1{"Thread 1"};
  TestMessage print2{"Thread 2"};
  TestMessage print3{"Thread 3"};
  paraos::Thread::StartScheduler();

  return 0;
}
