
#include <iostream>

#include "paraos_critical.hpp"
#include "paraos_thread_v2.hpp"

struct PrintTestMessage : public paraos::v2::Thread {
  PrintTestMessage(const std::string name = "default thread name")
      : paraos::v2::Thread{name, 1024, 4} {}
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

  paraos::v2::Thread::StartScheduler();
  paraos::v2::Thread::DeleteAll();
  return 0;
}