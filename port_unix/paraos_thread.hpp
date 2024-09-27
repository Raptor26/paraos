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

#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <deque>
#include <string>
#include <string_view>

#include "gsl/gsl"
#include "paraos_bool_atomic.hpp"
#include "paraos_check.h"
#include "paraos_config.hpp"
#include "paraos_critical.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_trace.hpp"

namespace paraos {

enum class ThreadPriority : int {
  kIdle = 1,
  kLowest,
  kBelowNormal,
  kNormal,
  kAboveNormal,
  kHighest,
  kRealTime,
};

class Thread {
 public:
  Thread(
      const std::string name, std::size_t stack_depth, ThreadPriority priority,
      bool is_joinable = true)
      : name_{std::move(name)},
        stack_depth_{stack_depth},
        priority_{priority},
        is_joinable_{is_joinable} {
    queue_thread_obj_.push_back(this);
    // Now Dtor can delete thread.
    is_thread_complete_sem_.Give();
  }

  virtual ~Thread() {
    // Dtor free resources only after thread body in perform_work()
    // complete execute.
    std::size_t delay_ms{4000};
    auto is_sem_taken = is_thread_complete_sem_.Take(delay_ms);

    PARAOS_CHECK_ASSERT(
        is_sem_taken &&
        "If you create thread, you must call Thread::StartScheduler() in "
        "main(), otherwise, destructor can't safely delete thread");

    PARAOS_ATTR_UNUSED_VAR(is_sem_taken);

    const paraos::CriticalSection critical;
    if (auto iter = std::find(
            queue_thread_obj_.cbegin(), queue_thread_obj_.cend(), this);
        iter != queue_thread_obj_.cend()) {
      int result{0};

      PARAOS_CHECK_ASSERT(result == 0 && "Error when try canceled thread");
      if (result == 0) {
        // Необходимо удалить дескриптор из очереди
        queue_thread_obj_.erase(iter);

        paraosTRACE_MESSAGE("Thread deleted: " << name_);

        is_thread_created_ = false;
      }
    } else {
// Повторное удаление уже удаленного потока. Данная ситуация может
// возникнуть когда вызвана функция DeleteAll(), а затем объекты потоков вышли
// из области видимости. В целом это не является ошибкой т.к. присутствует
// защита от повторного удаления потока
#if 1
      PARAOS_CHECK_ASSERT(
          false && "We can't find 'this' for thread delete operation");
#endif
    }
  }

  Thread(const Thread &other) = delete;
  Thread(Thread &&other) = delete;
  Thread &operator=(const Thread &other) = delete;
  Thread &operator=(Thread &&other) = delete;

  /// @brief After "Thread' Ctor complete construct object, user's inheritance
  /// class must call 'Start()' for create thread and scheduling this thread
  /// instance.
  auto Start() { return Make(); }

  auto Join() -> bool {
    int result_code{-1};
    if (is_joinable_ && is_thread_created_) {
      result_code = pthread_join(handle_, nullptr);
      PARAOS_CHECK_ASSERT(result_code == 0 && "Can't join the thread");
    }

    return result_code == 0 ? true : false;
  }

  std::string_view Name() { return name_; }

  void DelayMs(std::size_t sleep_ms) {
    usleep(sleep_ms * MICROSECONDS_PER_MILISECONDS);
  }

  PARAOS_INLINE_TRIVIAL void SetNeedWhile(bool is_need_while) {
    is_need_while_ = is_need_while;
  }

  bool SetPriority(const ThreadPriority priority) {
    bool is_priority_updated{false};

    PARAOS_CHECK_ASSERT(
        IsPriorityInRange(priority) == true &&
        "Priority out of range, use only ThreadPriority definitions for change "
        "priority");

    const paraos::CriticalSection critical;

    // Изменение приоритета потока возможно только в случае запуска программы от
    // имени суперпользователя
    if (IsRunAsRoot() == true) {
      int policy{0};
      sched_param sched{};
      if (pthread_getschedparam(handle_, &policy, &sched) != 0) {
        PARAOS_CHECK_ASSERT(
            false && "pthread_getschedparam() return error code");
      }

      sched.sched_priority = static_cast<int>(priority);
      auto prior_update_status =
          pthread_setschedparam(handle_, SCHED_RR, &sched);

      paraosTRACE_MESSAGE(
          "pthread_setschedparam return core: " << prior_update_status);

      if (prior_update_status == 0) {
        is_priority_updated = true;
      }
    } else {
      // Если запуск программы выполнен без прав суперпользователя, то мы не
      // можем изменить приоритет потока. В этом случае мы вернем флаг true для
      // обеспечения обратной совместимости
      is_priority_updated = true;
    }

    return is_priority_updated;
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

  static void StartScheduler() {
    paraosTRACE_MESSAGE("Start Scheduler");

    is_scheduler_started_ = true;

    for (auto &thread : queue_thread_obj_) {
      thread->sem_.Give();
    }

    for (auto &thread : queue_thread_obj_) {
      thread->Join();
    }
  }

  static void DeleteAll() {
    // Thread deleted in Dtor only.
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
    if (!is_thread_created_) {
      const paraos::CriticalSection critical;

      // Sem was given in Ctor. Now me take sem. That's mean, Dtor can delete
      // object only after perform_work() complete.
      constexpr std::size_t delay_ms{0u};
      auto is_sem_taken = is_thread_complete_sem_.Take(delay_ms);

      // If is_sem_taken == false, it's mean error in thread Ctor/Dtor logic.
      PARAOS_CHECK_ASSERT(is_sem_taken && "Sem always must taken");

      PARAOS_ATTR_UNUSED_VAR(is_sem_taken);

      auto result_code = pthread_create(&handle_, nullptr, perform_work, this);

      PARAOS_CHECK_ASSERT(result_code == 0 && "Thread not created");

      if (result_code == 0) {
        SetPriority(priority_);

        is_thread_created_ = true;

        if (IsSchedulerStarted()) {
          // Give semaphore, because scheduler already started. In this case
          // thread started after call Make().
          sem_.Give();
        }
      }
    }

    return is_thread_created_;
  }

  PARAOS_INLINE_TRIVIAL auto IsNeedWhile() const { return is_need_while_; }

  /// @brief Метод проверяет, выполнен ли запуск программны от имени
  /// суперпользователя.
  /// @note
  /// https://stackoverflow.com/questions/3214297/how-can-my-c-c-application-determine-if-the-root-user-is-executing-the-command
  /// @return
  bool IsRunAsRoot() {
    // В случае сборки под docker мы не используем права суперпользователя. Это
    // сделано для того чтобы SetPriority() всегда возвращало true
#if NOSUDO
    return false;
#else
    bool is_run_as_root{false};

    auto user = getuid();
    if (user == 0) {
      paraosTRACE_MESSAGE("Program run as root");
      is_run_as_root = true;
    } else {
      paraosTRACE_MESSAGE("No root");
    }
    return is_run_as_root;
#endif
  }

  bool IsPriorityInRange(ThreadPriority priority) {
    bool is_in_range{false};

    int policy;
    sched_param sched;
    pthread_getschedparam(handle_, &policy, &sched);

    auto min = sched_get_priority_min(policy);
    auto max = sched_get_priority_max(policy);
    auto prior = static_cast<int>(priority);

    if ((prior >= min) && (min <= max)) {
      is_in_range = true;
    }

    return is_in_range;
  }

  static void *perform_work(void *arguments) {
    Thread *thread = static_cast<Thread *>(arguments);

    // Need call StartScheduler() for give this semaphore.
    thread->sem_.Take(max_delay);

    thread->SetPriority(thread->priority_);

    // Нужно ли выполнение в теле бесконечного цикла задается при создании
    // потока в конструкторе ThreadBase()
    do {
      // Утверждение ниже сработает в том случае, если кто-то вызвал деструктор
      // для объекта типа 'Thread' (или его наследника). Это означает что время
      // жизни объекта меньше времени жизни потока, что является ошибкой.
      PARAOS_CHECK_ASSERT(
          !thread->is_canceled_ &&
          "Somebody call destruction for thread object");
      thread->Run();
    } while (thread->IsNeedWhile());

    // Atomic thread exit ------------------------------------------------------
    {
      const paraos::CriticalSection critical;

      if (!thread->is_canceled_) {
        // Необходимо пометить поток как отмененный чтобы деструктор объекта
        // повторно не удалил объект
        thread->is_canceled_ = true;
      }
    }

    // Give semaphore after perform_work() complete.
    auto after_return =
        gsl::finally([&] { thread->is_thread_complete_sem_.Give(); });

    return nullptr;
  }

 private:
  std::string name_;
  [[maybe_unused]] std::size_t stack_depth_{0};
  pthread_t handle_{0};
  ThreadPriority priority_{ThreadPriority::kIdle};
  BoolAtomic is_joinable_;

  /// @brief Флаг отмены потока. Если флаг установлен в true, то поток помечен
  /// как удаленный и в скором времени фактически будет удален.
  bool is_canceled_{false};

  /// @brief Данный флаг устанавливается в true если нужно вызывать Processing()
  /// в бесконечном цикле.
  BoolAtomic is_need_while_{false};

  /// @brief Sem for suspend thread if not call StartScheduler().
  SemaphoreBinary sem_;

  /// Global objects
 private:
  static inline std::deque<paraos::Thread *> queue_thread_obj_;
  static inline BoolAtomic is_scheduler_started_{false};

  /// @brief Set true after thread creation.
  BoolAtomic is_thread_created_{false};

  /// @brief If semaphore given, that's mean perform_work() complete execute and
  /// Dtor can safely free resources.
  SemaphoreBinary is_thread_complete_sem_;
};

}  // namespace paraos

#endif /* PARAOS_THREAD_HPP */
