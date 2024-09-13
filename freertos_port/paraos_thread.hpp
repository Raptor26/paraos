#ifndef PARAOS_THREAD_HPP
#define PARAOS_THREAD_HPP

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

#include <cstring>
#include <deque>
#include <string>

#include "FreeRTOS.h"
#include "gsl/gsl"
#include "paraos_mutex.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_trace.hpp"
#include "paraos_utils.hpp"

namespace paraos {

/// @brief Перечисление видов приоритетов у потоков freeRTOS.
enum class ThreadPriority : int {
  kIdle = 0,
  kLowest,
  kBelowNormal,
  kNormal,
  kAboveNormal,
  kHighest,
  kRealTime,

  kMaxNum
};

/// @brief Класс-реализация потоков freeRTOS.
class Thread {
 public:
  /// @brief Конструктор потока freeRTOS.
  /// @param[in] name: Название потока.
  /// @param[in] stack_depth: Размер стека потока в байтах.
  /// @param[in] priority: Приоритет создаваемого потока.
  /// @param[in] is_joinable: Флаг закрепления создаваемого потока за главным
  /// потоком.
  Thread(
      const std::string name, std::size_t stack_depth, ThreadPriority priority,
      bool is_joinable = false)
      : name_{name},
        stack_depth_{stack_depth},
        priority_{priority},
        is_joinable_{is_joinable} {
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

    const paraos::CriticalSection critical;

    if (handle_) {
      // Если поток не удалял себя из планировщика, будучи откреплённым от
      // основного потока.
      if (is_thread_created_) {
        vTaskDelete(handle_);
      }

      // Необходимо сбросить дескриптор потока с целью избежать повторного
      // удаления потока
      handle_ = nullptr;

      paraosTRACE_MESSAGE("Thread deleted: " << name_);

      is_thread_created_ = false;
    }
  }

  /// @brief Метод используется для получения названия потока.
  /// @return Возвращает имя потока.
  std::string_view Name() { return name_; }
  /// @brief Метод используется для получения дескриптора созданного потока.
  /// @return Возвращает дескриптор потока.
  TaskHandle_t Handle() { return handle_; }

  /// @brief Метод используется для задержки потока на указанное время.
  /// @param[in] sleep_ms: Время в мс, на которое необходимо заблокировать
  /// поток.
  void DelayMs(std::size_t sleep_ms) {
    vTaskDelay(static_cast<TickType_t>(RTOS_THREAD_ConvertMsToTicks(sleep_ms)));
  }

  /// @brief After "Thread' complete construct object, user's inheritance
  /// class must call 'Start()' for create thread and scheduling this thread
  /// instance.
  void Start() {
    // if scheduler already started, thread will be created in running state.
    Make();
  }

  /// @brief Метод используется для ожидания основной программой завершения
  /// потока.
  /// @return Возвращает результат операции.
  auto Join() -> bool {
    bool join_result = true;

    /* Make sure pthread is joinable. Otherwise, this function would block
     * forever waiting for an unjoinable thread. */
    if (!is_joinable_ || !is_thread_created_) {
      join_result = false;
    }

    /* Only one thread may attempt to join another. Lock the join mutex
     * to prevent other threads from calling pthread_join on the same thread. */
    if (join_result == true) {
      if (join_mutex_.Lock(0) != true) {
        /* Another thread has already joined the requested thread, which would
         * cause this thread to wait forever. */
        join_result = false;
      }
    }

    if (join_result == true) {
      /* Wait for the joining thread to finish. Because this call waits forever,
       * it should never fail. */
      is_thread_complete_sem_.Take();

      /* Create a critical section to clean up the joined thread. */
      const paraos::CriticalSection critical;

      is_thread_complete_sem_.Give();

      join_mutex_.Unlock();

      /* End the critical section. */
    }

    return join_result;
  }

  /// @brief Метод используется для открепления потока от основной программы.
  /// @return Возвращает результат операции.
  auto Detach() -> bool {
    bool detach_result = false;
    if (is_joinable_ == true && is_thread_created_ == true) {
      const paraos::CriticalSection critical;
      auto thread_state = eTaskGetState(handle_);
      if (thread_state != eDeleted && thread_state != eInvalid) {
        if (thread_state == eSuspended) {
          is_thread_created_ = false;
          detach_result = true;
          vTaskDelete(handle_);
        } else {
          is_joinable_ = false;
          detach_result = true;
        }
      }
    }

    return detach_result;
  }

  /// @brief Метод run необходимо переопределять в классах, наследниках для
  /// реализации логики работы потока.
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

  /// @brief Метод выполняет запуск планировщика.
  ///@note Планировщик должен запускаться в главном потоке.
  static void StartScheduler() {
    paraosTRACE_MESSAGE("Start Scheduler");

    is_scheduler_started_ = true;

    vTaskStartScheduler();
  }

  /// @brief Метод выполняет остановку планировщика.
  static void StopScheduler() {
    paraosTRACE_MESSAGE("Stop Scheduler");
    is_scheduler_started_ = false;
    vTaskEndScheduler();
  }

  /// @brief Метод выполняет удаление потоков и остановку планировщика.
  /// @note Используется в тестах.
  static void DeleteAll() {
    const paraos::CriticalSection critical;
    while (!queue_thread_obj_.empty()) {
      // Мы получаем ссылку на элемент в очереди, при этом при вызове front()
      // элемент из очереди не удаляется
      auto &thread_ptr = queue_thread_obj_.front();

      thread_ptr->~Thread();

      queue_thread_obj_.pop_front();

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
  }

  /// @brief Метод выполняет изменение приоритета потока.
  /// @param[in] priority: Новый приоритет потока.
  /// @return ВОзвращает  результат выполнения операции.
  bool SetPriority(const ThreadPriority priority) {
    bool result = false;

    if (priority < ThreadPriority::kMaxNum) {
      priority_ = priority;
      vTaskPrioritySet(handle_, static_cast<UBaseType_t>(priority));
      result = true;
    }

    return result;
  }

  /// @brief Метод устанавливает флаг необходимости вызова в цикле while метода
  /// Run() у потока.
  /// @param[in] status: Новое значение флага.
  /// @return
  auto SetNeedWhile(bool status) -> bool {
    is_need_while_ = status;

    return true;
  }

  /// @brief Метод используется для получения информации о запуске планировщика.
  /// @return Возвращает статус планировщика - запущен или нет.
  static auto IsSchedulerStarted() { return is_scheduler_started_; }

  Thread(const Thread &other) = delete;
  Thread(Thread &&other) = delete;
  Thread &operator=(const Thread &other) = delete;
  Thread &operator=(Thread &&other) = delete;

 private:
  void Make() {
    // Guard to prevent double thread creation for single 'Thread' object.
    if (!is_thread_created_) {
      // Sem was given in Ctor. Now we take sem. That's mean, Dtor can delete
      // object only after MyThreadFunction() complete.
      constexpr std::size_t delay_ms{0u};
      auto is_sem_taken = is_thread_complete_sem_.Take(delay_ms);

      // If is_sem_taken == false, it's mean error in thread Ctor/Dtor logic.
      PARAOS_CHECK_ASSERT(is_sem_taken && "Sem always must taken");

      xTaskCreate(
          MyThreadFunction, name_.c_str(), stack_depth_, this,
          static_cast<UBaseType_t>(priority_), &handle_);

      PARAOS_CHECK_ASSERT(handle_ && "Thread not created");
      is_thread_created_ = true;

      const paraos::CriticalSection critical;
      queue_thread_obj_.push_back(this);
    }
  }

  inline auto IsNeedWhile() const { return is_need_while_; }

  void ExitThread() {
    is_thread_complete_sem_.Give();
    is_thread_created_ = false;
    vTaskDelete(nullptr);
  }

  static void MyThreadFunction(void *lpParam) {
    Thread *thread = static_cast<Thread *>(lpParam);

    // Нужно ли выполнение в теле бесконечного цикла задается при создании
    // потока в конструкторе ThreadBase()
    do {
      thread->Run();
    } while (thread->IsNeedWhile());

    thread->ExitThread();
  }

 private:
  std::string name_;
  std::size_t stack_depth_{0};
  TaskHandle_t handle_{nullptr};
  ThreadPriority priority_{ThreadPriority::kIdle};
  [[maybe_unused]] BoolAtomic is_joinable_{false};

  /// @brief Данный флаг устанавливается в true если нужно вызывать
  /// Processing() в бесконечном цикле.
  [[maybe_unused]] BoolAtomic is_need_while_{false};

 private:
  static inline std::deque<paraos::Thread *> queue_thread_obj_;
  static inline BoolAtomic is_scheduler_started_{false};

  /// @brief Set true after thread creation.
  [[maybe_unused]] BoolAtomic is_thread_created_{false};

  /// @brief If semaphore given, that's mean perform_work() complete execute and
  /// Dtor can safely free resources.
  SemaphoreBinary is_thread_complete_sem_;
  MutexBaseBinary join_mutex_;
};
}  // namespace paraos

#endif /* PARAOS_THREAD_HPP */
