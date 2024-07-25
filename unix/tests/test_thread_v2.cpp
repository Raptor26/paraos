
#include <iostream>

#include "paraos_critical.hpp"
#include "paraos_thread_v2.hpp"

struct PrintTestMessage : public paraos::v2::Thread {
  PrintTestMessage(const std::string name = "default thread name")
      : paraos::v2::Thread{
            name, 1024, paraos::v2::ThreadPriority::kBelowNormal} {}
  void Run() override {
    const paraos::CriticalSection critical;
    std::cout << Name() << " Cnt is " << cnt_ << std::endl;
    ++cnt_;
  }

 private:
  std::size_t cnt_{0};
};

int main() {
  PrintTestMessage print1{"Thread 1"};
  PrintTestMessage print2{"Thread 2"};
  PrintTestMessage print3{"Thread 3"};
  PrintTestMessage print4{"Thread 4"};
  PrintTestMessage print5{"Thread 5"};
  PrintTestMessage print6{"Thread 6"};

  PrintTestMessage print11{"Thread 11"};
  PrintTestMessage print22{"Thread 22"};
  PrintTestMessage print33{"Thread 33"};
  PrintTestMessage print44{"Thread 44"};
  PrintTestMessage print55{"Thread 55"};
  PrintTestMessage print66{"Thread 66"};

  paraos::v2::Thread::StartScheduler();
  paraos::v2::Thread::DeleteAll();
  return 0;
}