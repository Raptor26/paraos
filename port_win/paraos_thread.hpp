/// @file paraos_thread.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2024 Stilsoft
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

#ifndef PARAOS_THREAD_HPP
#define PARAOS_THREAD_HPP

#include <processthreadsapi.h>

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

#include "gsl/gsl"
#include "paraos_attr.h"
#include "paraos_bool_atomic.hpp"
#include "paraos_config.hpp"
#include "paraos_critical.hpp"
#include "paraos_mutex.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_semaphore.hpp"
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
  Thread(
      const std::string name, std::size_t stack_depth, int priority,
      bool is_joinable = true)
      : name_{name},
        stack_depth_{stack_depth},
        priority_{priority},
        is_joinable_{is_joinable} {
    // Now Dtor can delete thread.
    is_thread_complete_sem_.Give();
  }

  virtual ~Thread() {
    // Destructor initialize competition thread loop for safety destruct object.
    SetNeedWhile(false);

    // Dtor free resources only after thread body in perform_work()
    // complete execute.
    std::size_t delay_ms{4000};
    auto is_sem_taken = is_thread_complete_sem_.Take(delay_ms);

    // Suppress warnings when release build and PARAOS_CHECK_ASSERT() is
    // disable.
    PARAOS_ATTR_UNUSED_VAR(is_sem_taken);

    PARAOS_CHECK_ASSERT(
        is_sem_taken &&
        "If you create thread, you must call Thread::StartScheduler() in "
        "main(), otherwise, destructor can't safely delete thread");

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

          is_thread_created_ = false;
        }
      }
    } else {
// Повторное удаление уже удаленного потока. Данная ситуация может
// возникнуть когда вызвана функция DeleteAll(), а затем объекты потоков вышли
// из области видимости. В целом это не является ошибкой т.к. присутствует
// защита от повторного удаления потока
#if 0
        PARAOS_CHECK_ASSERT(false && "We can't find 'this' for thread delete operation");
#endif
    }
  }

  Thread(const Thread &other) = delete;
  Thread(Thread &&other) = delete;
  Thread &operator=(const Thread &other) = delete;
  Thread &operator=(Thread &&other) = delete;

  /// @brief After "Thread' complete construct object, user's inheritance
  /// class must call 'Start()' for create thread and scheduling this thread
  /// instance.
  auto Start() {
    // if scheduler already started, thread will be created in running state.
    return Make();
  }

  auto Join() -> bool {
    bool is_joined{false};
    // Поток можно присоединить только в том случае, если он не был присоединен
    // ранее
    if (is_joinable_ && is_thread_created_) {
      auto status = WaitForSingleObject(handle_, INFINITE);

      PARAOS_CHECK_ASSERT(status == WAIT_OBJECT_0 && "Can't join thread");

      /// @see
      /// https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
      if (status == WAIT_OBJECT_0) {
        is_joined = true;

        // After join, thread can't be joinable later.
        is_joinable_ = false;
        paraosTRACE_MESSAGE("Thread join: " << name_);
      }
    }

    return is_joined;
  }

  std::string_view Name() { return name_; }

  bool SetPriority(const ThreadPriority priority) {
    auto is_priority_updated =
        SetThreadPriority(handle_, static_cast<int>(priority));

    if (is_priority_updated) {
      priority_ = priority;
    }
    return is_priority_updated;
  }

  void DelayMs(std::size_t sleep_ms) const { Sleep(sleep_ms); }

  static void SleepMs(std::size_t sleep_ms) { Sleep(sleep_ms); }

  /// @brief Enable or disable loop calling Run() method.
  /// @param[in] is_need_while: if `is_need_while == true`, Run() calling in
  /// infinite loop. If set `is_need_while == false`, Run() calling at once.
  PARAOS_INLINE_TRIVIAL void SetNeedWhile(bool is_need_while) {
    is_need_while_ = is_need_while;
  }

  /// @brief Return current tine in ticks. Useful when need periodical check
  /// timeout in blocking operations with elapsed time correction.
  ///
  /// @return Return object with current time. Returned value used in
  /// CheckTimeout().
  static auto GetCurrentTime() -> OsProfiler {
    // Create profiler and capture current time.
    OsProfiler profiler;
    profiler.Start();
    return profiler;
  }

  /// @brief Check timeout with elapsed time correction.
  ///
  /// @details If a task enters and exits the Blocked state more than once while
  /// it is waiting for the event to occur then the timeout used each time the
  /// task enters the Blocked state must be adjusted to ensure the total of all
  /// the time spent in the Blocked state does not exceed the originally
  /// specified timeout period. xTaskCheckForTimeOut() performs the adjustment,
  /// taking into account occasional occurrences such as tick count overflows,
  /// which would otherwise make a manual adjustment prone to error.
  ///
  /// @param[in] timeout: Returned by GetCurrentTime() value.
  /// GetCurrentTimeInTicks() using at once before need periodical checking
  /// timeout by CheckTimeout().
  /// @param[in,out] delay_ms: Wait time in [ms]. Note: In windows port delay_ms
  /// not modifed, by other ports (freeRTOS for example), delay_ms modify each
  /// CheckTimeout() call.
  ///
  /// @return Return true if need break waiting, false if no timeout elapsed.
  static auto CheckTimeout(OsProfiler &timeout, std::size_t &delay_ms) -> bool {
    const CriticalSection critical;
    bool is_timeout{true};
    timeout.Stop();

    auto elapsed_time = timeout.LastDurationMs();

    if (delay_ms > elapsed_time) {
      is_timeout = false;

      // Reduced delay_ms. It's need for caller, which can again enter in
      // blocking mode with updated timeout.
      delay_ms -= elapsed_time;

      // Update start point because delay_ms was modified. It's necessary for
      // correct update delay_ms if CheckTimeout() will call again.
      timeout.Start();
    }

    return is_timeout;
  }

  virtual void Run() {
    // Если сработал данный PARAOS_CHECK_ASSERT, то конструктор производного от
    // Thread класса не успел завершить конструирование объекта до того момента
    // когда планировщик ОС вызвал метод Run() (производные классы всегда должны
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
    PARAOS_CHECK_ASSERT(
        false &&
        "If windows scheduler call this instance, constructor of derived class "
        "not complete its work before scheduler call Run() method");
  };

  /// @brief StartScheduler() must run in main thread.
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
#if 0
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
    PARAOS_CHECK_ASSERT(
        queue_thread_obj_.empty() &&
        "Container for pointers threadable objects must be empty, otherwise "
        "some thread not deleted");
#endif
  }

  static auto IsSchedulerStarted() { return is_scheduler_started_; }

 private:
  auto Make() -> bool {
    // Guard to prevent double thread creation for single 'Thread' object.
    if (!is_thread_created_) {
      // Sem was given in Ctor. Now we take sem. That's mean, Dtor can delete
      // object only after MyThreadFunction() complete.
      constexpr std::size_t delay_ms{0u};
      auto is_sem_taken = is_thread_complete_sem_.Take(delay_ms);

      // Suppress warnings when release build and PARAOS_CHECK_ASSERT() is
      // disable.
      PARAOS_ATTR_UNUSED_VAR(is_sem_taken);

      // If is_sem_taken == false, it's mean error in thread Ctor/Dtor logic.
      PARAOS_CHECK_ASSERT(is_sem_taken && "Sem always must taken");

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

      PARAOS_CHECK_ASSERT(handle_ && "Thread not created");
      is_thread_created_ = true;

      const paraos::CriticalSection critical;
      queue_thread_obj_.push_back(this);
    }

    return is_thread_created_;
  }

  PARAOS_INLINE_TRIVIAL auto IsNeedWhile() const { return is_need_while_; }

  static DWORD WINAPI MyThreadFunction(LPVOID lpParam) {
    Thread *thread = static_cast<Thread *>(lpParam);

    thread->SetPriority(thread->priority_);

    // Нужно ли выполнение в теле бесконечного цикла задается при создании
    // потока в конструкторе ThreadBase()
    do {
      thread->Run();
    } while (thread->IsNeedWhile());

    // Give semaphore after MyThreadFunction() complete.
    auto after_return =
        gsl::finally([&] { thread->is_thread_complete_sem_.Give(); });

    // Если бы использовался freeRTOS, то вызвали "vTaskDelete(nullptr)"
    return 0;
  }

 private:
  std::string name_;
  size_t stack_depth_{0};
  HANDLE handle_{nullptr};
  DWORD thread_id_{0};

  /// @brief Since thread priority set outside the construction, it's necessary
  /// to buffer priority value when user call constructor.
  ThreadPriority priority_{ThreadPriority::kIdle};

  BoolAtomic is_joinable_;

  /// @brief Данный флаг устанавливается в true если нужно вызывать Processing()
  /// в бесконечном цикле.
  BoolAtomic is_need_while_{false};

  /// Global objects
 private:
  static inline std::deque<paraos::Thread *> queue_thread_obj_;
  static inline BoolAtomic is_scheduler_started_{false};

  /// @brief Set true after thread creation.
  BoolAtomic is_thread_created_{false};

  /// @brief If semaphore given, that's mean perform_work() complete execute and
  /// Dtor can safely free resources.
  SemaphoreBinary is_thread_complete_sem_;

  OsProfiler current_time_;
};

}  // namespace paraos

#endif /* PARAOS_THREAD_HPP */
