#ifndef paraos_thread_HPP
#define paraos_thread_HPP

#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <deque>
#include <string>

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
      const std::string name, std::size_t stack_depth, ThreadPriority priority)
      : name_{std::move(name)}, stack_depth_{stack_depth}, priority_{priority} {
    Make();
  }

  virtual ~Thread() {
    const paraos::CriticalSection critical;
    if (auto iter = std::find(
            queue_thread_obj_.cbegin(), queue_thread_obj_.cend(), this);
        iter != queue_thread_obj_.cend()) {
      int result{0};

      // Поток можно принудительно удалить только в том случае, если он не был
      // удален ранее. Поток самостоятельно удаляет себя в конце тела функции
      // perform_work()
      if (!is_canceled_) {
        Thread *thread_ptr = *iter;
        result = pthread_cancel(thread_ptr->handle_);
        is_canceled_ = true;
      }

      assert(result == 0 && "Error when try canceled thread");
      if (result == 0) {
        // Необходимо удалить дескриптор из очереди
        queue_thread_obj_.erase(iter);

        paraosTRACE_MESSAGE("Thread deleted: " << name_);
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

  void Join() { auto result_code = pthread_join(handle_, nullptr); }

  std::string_view Name() { return name_; }

  bool SetPriority(const ThreadPriority priority) {
    bool is_priority_updated{false};

    const paraos::CriticalSection critical;

    assert(
        IsPriorityInRange(priority) == true &&
        "Priority out of range, use only ThreadPriority definitions for change "
        "priority");

    // Изменение приоритета потока возможно только в случае запуска программы от
    // имени суперпользователя
    if (IsRunAsRoot() == true) {
      int policy{0};
      sched_param sched{};
      if (pthread_getschedparam(handle_, &policy, &sched) != 0) {
        assert(false && "pthread_getschedparam() return error code");
      }

      sched.sched_priority = static_cast<int>(priority);
      auto prior_update_status =
          pthread_setschedparam(handle_, SCHED_RR, &sched);

      paraosTRACE_MESSAGE(
          "pthread_setschedparam return core: " << prior_update_status);

      assert(
          prior_update_status == 0 &&
          "SetPriority() not set new thread priority");

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

    for (auto &thread : queue_thread_obj_) {
      thread->sem_.Give();
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
    auto result_code = pthread_create(&handle_, nullptr, perform_work, this);

    if (result_code == 0) {
      const paraos::CriticalSection critical;
      auto is_priority_set = SetPriority(priority_);
      assert(is_priority_set == true && "Priority not updated");
      queue_thread_obj_.push_back(this);
    }
  }

  PARAOS_INLINE_TRIVIAL auto IsNeedWhile() const { return is_need_while_; }

  /// @brief Метод проверяет, выполнен ли запуск программны от имени
  /// суперпользователя.
  /// @note
  /// https://stackoverflow.com/questions/3214297/how-can-my-c-c-application-determine-if-the-root-user-is-executing-the-command
  /// @return
  bool IsRunAsRoot() {
    bool is_run_as_root{false};

    auto user = getuid();
    if (user == 0) {
      paraosTRACE_MESSAGE("Program run as root");
      is_run_as_root = true;
    } else {
      paraosTRACE_MESSAGE("No root");
    }
    return is_run_as_root;
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

    thread->sem_.Take(max_delay);

    auto is_priority_set = thread->SetPriority(thread->priority_);
    assert(is_priority_set == true && "Priority not updated");
    (void)is_priority_set;

    // Запишем в локальную переменную значение флага. Это позволит избежать
    // операции разыменование указатели при работе в теле цикла do ->
    // while()
    const auto is_need_while = thread->IsNeedWhile();

    // Нужно ли выполнение в теле бесконечного цикла задается при создании
    // потока в конструкторе ThreadBase()
    do {
      // Утверждение ниже сработает в том случае, если кто-то вызвал деструктор
      // для объекта типа 'Thread' (или его наследника). Это означает что время
      // жизни объекта меньше времени жизни потока, что является ошибкой.
      assert(
          !thread->is_canceled_ &&
          "Somebody call destruction for thread object");
      thread->Run();
    } while (is_need_while);

    // Atomic thread exit ------------------------------------------------------
    const paraos::CriticalSection critical;

    assert(
        !thread->is_canceled_ && "Somebody call destruction for thread object");

    if (!thread->is_canceled_) {
      // Необходимо пометить поток как отмененный чтобы деструктор объекта
      // повторно не удалил объект
      thread->is_canceled_ = true;
    }

    // Несмотря на состояние потока, при завершении его тела функции необходимо
    // вызвать строку ниже
    pthread_exit(PTHREAD_CANCELED);
  }

 private:
  std::string name_;
  std::size_t stack_depth_{0};
  pthread_t handle_{0};
  BoolSafeThreadFlag is_joinable_{true};
  ThreadPriority priority_{ThreadPriority::kIdle};

  /// @brief Флаг отмены потока. Если флаг установлен в true, то поток помечен
  /// как удаленный и в скором времени фактически будет удален.
  bool is_canceled_{false};

  /// @brief Данный флаг устанавливается в true если нужно вызывать Processing()
  /// в бесконечном цикле.
  bool is_need_while_{false};

  Semaphore sem_;

  /// Global objects
 private:
  static inline std::deque<paraos::Thread *> queue_thread_obj_;
  static inline BoolSafeThreadFlag is_scheduler_started_{false};
};

}  // namespace paraos

#endif /* paraos_thread_HPP */
