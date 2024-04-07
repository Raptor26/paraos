#include <functional>
#include <iostream>

#include "thread.hpp"

class Executor {
 public:
  bool Execute(bool someFlag) {
    std::cout << "Execute " << someFlag << std::endl;

    return true;
  }
};

class MyClassWithProcessing {
 private:
  int cnt = 2;

 public:
  void Processing() {
    std::cout << "Processing " << cnt << std::endl;
    ++cnt;
  }
};

template <typename TMemberFunction>
class Invoker;

template <class C, typename Ret, typename... Args>
class Invoker<Ret (C::*)(Args...)> {
 public:
  Ret Invoke(Ret (C::*method)(Args...), C* instance, Args... args) {
    return std::invoke(method, instance, std::forward<Args>(args)...);
  }
};

// template <class C, typename Ret, typename... Args>
// class Invoker<Ret (C::*)(Args...) const> {
//  public:
//   Ret Invoke(Ret (C::*method)(Args...) const, const C* instance, Args...
//   args) {
//     return std::invoke(method, instance, std::forward<Args>(args)...);
//   }
// };
// other specializations to handle combination of volatile, ref, c-ellipsis

int main() {
  Executor executor;
  Invoker<bool (Executor::*)(bool)> invoker;
  bool response = invoker.Invoke(&Executor::Execute, &executor, true);
  invoker.Invoke(&Executor::Execute, &executor, false);

  using namespace paraos;
  Thread thread_factory;

  thread_factory.Invoke(&Executor::Execute, &executor, true);

  MyClassWithProcessing my_pseudo_thread;

  thread_factory.Invoke(&MyClassWithProcessing::Processing, &my_pseudo_thread);
  return 0;
}