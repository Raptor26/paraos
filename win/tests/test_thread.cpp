
#include <iostream>

#include "paraos_critical.hpp"
#include "paraos_thread.hpp"

struct PrintTestMessage : public paraos::Thread {
  PrintTestMessage(const std::string name = "default thread name")
      : paraos::Thread{name, 1024, paraos::ThreadPriority::kLowest} {}
  void Run() override {
    const paraos::CriticalSection critical;
    std::cout << Name() << " Cnt is " << cnt_ << std::endl;
    ++cnt_;
  }

 private:
  size_t cnt_{0};
};

int main() {
  PrintTestMessage print1{"Thread 1"};
  PrintTestMessage print2{"Thread 2"};
  PrintTestMessage print3{"Thread 3"};

  paraos::Thread::StartScheduler();
  paraos::Thread::DeleteAll();
  return 0;
}