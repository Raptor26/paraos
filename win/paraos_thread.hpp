#ifndef paraos_thread_HPP
#define paraos_thread_HPP

#include <windows.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <functional>
#include <iostream>
#include <new>
#include <queue>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "paraos_config.hpp"
#include "paraos_critical.hpp"
#include "paraos_mutex.hpp"
#include "paraos_trace.hpp"

namespace paraos {

enum ThreadPriority : int {
  kIdle = THREAD_PRIORITY_IDLE,
  kLowest = THREAD_PRIORITY_LOWEST,
  kBelowNormal = THREAD_PRIORITY_BELOW_NORMAL,
  kNormal = THREAD_PRIORITY_NORMAL,
  kAboveNormal = THREAD_PRIORITY_ABOVE_NORMAL,
  kHighest = THREAD_PRIORITY_HIGHEST,
  kRealTime = THREAD_PRIORITY_TIME_CRITICAL,
};

class Thread {
 public:
  Thread(const std::string name, size_t stack_depth, int priority)
      : name_{name}, stack_depth_{stack_depth}, priority_{priority} {
    Make();
  }

  virtual ~Thread() {
    const paraos::CriticalSection critical;
    if (auto iter = std::find(
            queue_thread_obj_.cbegin(), queue_thread_obj_.cend(), this);
        iter != queue_thread_obj_.cend()) {
      Thread *thread_ptr = *iter;

      if (thread_ptr->handle_) {
        if (CloseHandle(thread_ptr->handle_) == true) {
          // Необходимо удалить дескриптор из очереди
          queue_thread_obj_.erase(iter);

          // Необходимо сбросить дескриптор потока с целью избежать повторного
          // удаления потока
          thread_ptr->handle_ = nullptr;

          paraosTRACE_MESSAGE("Thread deleted: " << name_);
        }
      }
    } else {
// Повторное удаление уже удаленного потока. Данная ситуация может
// возникнуть когда вызвана функция DeleteAll(), а затем объекты потоков вышли
// из области видимости. В целом это не является ошибкой т.к. присутствует
// защита от повторного удаления потока
#if 0
        assert(false && "We can't find 'this' for thread delete operation");
#endif
    }
  }

  Thread(const Thread &other) = delete;
  Thread(Thread &&other) = delete;
  Thread &operator=(const Thread &other) = delete;
  Thread &operator=(Thread &&other) = delete;

  void Join() {
    // Поток можно присоединить только в том случае, если он не был присоединен
    // ранее
    if (is_joinable_ == true) {
      is_joinable_ = false;

      auto status = WaitForSingleObject(handle_, INFINITE);

      /// @see
      /// https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
      if (status == WAIT_OBJECT_0) {
        paraosTRACE_MESSAGE("Thread join: " << name_);
      }
    }
  }

  std::string_view Name() { return name_; }

  bool SetPriority(const ThreadPriority priority) {
    return SetThreadPriority(handle_, static_cast<int>(priority));
  }

  virtual void Run() {
    // Если сработал данный assert, то конструктор производного от Thread класса
    // не успел завершить конструирование объекта до того момента когда
    // планировщик ОС вызвал метод Run() (производные классы всегда должны
    // переопределять метод Run()). Одним из возможных способов решения
    // являются:
    // - Переопределите в производном классе метод Run(). Это самый тривиальный
    //   случай. Возможно вы просто забыли определить тело вашего потока в
    //   методе Run().
    //
    // Пункты ниже рассматривайте только в том случае, если в производном классе
    // определен метод Run() с аннотацией override:
    //
    // - Вызов метода Make() в теле конструктора производного класса. Это
    //   гарантирует, что поток создается после того, как компилятор подставил
    //   указатель на метод Run() из производного класса.
    //
    // - Создание потока в приостановленном состоянии, затем его запуск в теле
    //   конструктора производного класса. Для данного сценария рассуждения
    //   аналогичны пункту выше.
    //
    // - Временное повышение приоритета потока, который создает новый поток. Это
    //   гарантирует, что создающий поток завершит работу конструкторов до того
    //   как планировщик ОС выполнит переключение на выполнение потока
    //   созданного объекта. После завершения создания объекта и его потока,
    //   создающий поток вновь может понизить свой приоритет до исходного
    //   значения.
    assert(
        false &&
        "If windows scheduler call this instance, constructor of derived class "
        "not complete its work before scheduler call Run() method");
  };

  static void StartScheduler() {
    paraosTRACE_MESSAGE("Start Scheduler");

    is_scheduler_started_ = true;

    // Потоки создаются в приостановленном состоянии. Необходимо возобновить
    // выполнение созданных потоков, а затем вызвать Join()
    for (auto &thread : queue_thread_obj_) {
      ResumeThread(thread->handle_);
    }

    for (auto &thread : queue_thread_obj_) {
      thread->Join();
    }
  }

  static void DeleteAll() {
    const paraos::CriticalSection critical;
    while (!queue_thread_obj_.empty()) {
      // Мы получаем ссылку на элемент в очереди, при этом при вызове front()
      // элемент из очереди не удаляется
      auto &thread_ptr = queue_thread_obj_.front();

      thread_ptr->~Thread();

      // нет необходимости вызывать pop() с целью удаления объекта потока из
      // очереди для queue_thread_obj_. Деструктор ~Thread() самостоятельно
      // удалит ссылку на себя из очереди
    }

    // Если сработало утверждение ниже, то возможно это связано с тем, что в
    // момент извлечения крайнего дескриптора потока из очереди, другой поток
    // поместил новый объект в очередь (критическая секция позволяет избежать
    // подобного состояния)
    assert(
        queue_thread_obj_.empty() &&
        "Container for pointers threadable objects must be empty, otherwise "
        "some thread not deleted");
  }

 private:
  void Make() {
    DWORD creation_flags{CREATE_SUSPENDED};

    // После запуска планировщика нет необходимости создавать потоки в
    // приостановленном состоянии
    if (is_scheduler_started_) {
      creation_flags = 0;
    }

    handle_ = CreateThread(
        NULL,                            // default security attributes
        stack_depth_,                    // use default stack size
        MyThreadFunction,                // thread function name
        reinterpret_cast<LPVOID>(this),  // argument to thread function
        creation_flags,                  // use default creation flags
        &thread_id_);                    // returns the thread identifier

    assert(handle_ && "Thread not created");

    const paraos::CriticalSection critical;
    queue_thread_obj_.push_back(this);
  }

  PARAOS_INLINE_TRIVIAL auto IsNeedWhile() const { return is_need_while_; }

  static DWORD WINAPI MyThreadFunction(LPVOID lpParam) {
    Thread *thread = static_cast<Thread *>(lpParam);

    auto is_priority_set = thread->SetPriority(thread->priority_);
    assert(is_priority_set == true && "Priority not updated");
    (void)is_priority_set;

    // Запишем в локальную переменную значение флага. Это позволит избежать
    // операции разыменование указатели при работе в теле цикла do -> while()
    const auto is_need_while = thread->IsNeedWhile();

    // Нужно ли выполнение в теле бесконечного цикла задается при создании
    // потока в конструкторе ThreadBase()
    do {
      thread->Run();
    } while (is_need_while);

    // Если бы использовался freeRTOS, то вызвали "vTaskDelete(nullptr)"
    return 0;
  }

 private:
  std::string name_;
  size_t stack_depth_{0};
  HANDLE handle_{nullptr};
  DWORD thread_id_{0};
  BoolSafeThreadFlag is_joinable_{true};
  ThreadPriority priority_{ThreadPriority::kIdle};

  /// @brief Данный флаг устанавливается в true если нужно вызывать Processing()
  /// в бесконечном цикле.
  bool is_need_while_{false};

  /// Global objects
 private:
  static inline std::deque<paraos::Thread *> queue_thread_obj_;
  static inline BoolSafeThreadFlag is_scheduler_started_{false};
};

}  // namespace paraos

#endif /* paraos_thread_HPP */
