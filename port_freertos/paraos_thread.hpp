#ifndef PARAOS_THREAD_HPP
#define PARAOS_THREAD_HPP

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

#include <cstring>
#include <deque>
#include <string>

#include "FreeRTOS.h"
#include "paraos_mutex.hpp"
#include "paraos_semaphore.hpp"
#include "task.h"

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
      bool is_joinable = false);

  virtual ~Thread();

  /// @brief Метод используется для получения названия потока.
  /// @return Возвращает имя потока.
  std::string_view Name();

  /// @brief Метод используется для получения дескриптора созданного потока.
  /// @return Возвращает дескриптор потока.
  TaskHandle_t Handle();

  /// @brief Метод используется для задержки потока на указанное время.
  /// @param[in] sleep_ms: Время в мс, на которое необходимо заблокировать
  /// поток.
  void DelayMs(std::size_t sleep_ms);

  /// @brief After "Thread' complete construct object, user's inheritance
  /// class must call 'Start()' for create thread and scheduling this thread
  /// instance.
  void Start();

  /// @brief Метод используется для ожидания основной программой завершения
  /// потока.
  /// @return Возвращает результат операции.
  auto Join() -> bool;

  /// @brief Метод используется для открепления потока от основной программы.
  /// @return Возвращает результат операции.
  auto Detach() -> bool;

  /// @brief Метод run необходимо переопределять в классах, наследниках для
  /// реализации логики работы потока.
  virtual void Run();

  /// @brief Метод выполняет запуск планировщика.
  ///@note Планировщик должен запускаться в главном потоке.
  static void StartScheduler();

  /// @brief Метод выполняет остановку планировщика.
  static void StopScheduler();

  /// @brief Метод выполняет удаление потоков и остановку планировщика.
  /// @note Используется в тестах.
  static void DeleteAll();

  /// @brief Метод выполняет изменение приоритета потока.
  /// @param[in] priority: Новый приоритет потока.
  /// @return ВОзвращает  результат выполнения операции.
  bool SetPriority(const ThreadPriority priority);

  /// @brief Метод устанавливает флаг необходимости вызова в цикле while метода
  /// Run() у потока.
  /// @param[in] status: Новое значение флага.
  /// @return
  auto SetNeedWhile(bool status) -> bool;

  /// @brief Метод используется для получения информации о запуске планировщика.
  /// @return Возвращает статус планировщика - запущен или нет.
  static auto IsSchedulerStarted() -> bool;

  Thread(const Thread &other) = delete;
  Thread(Thread &&other) = delete;
  Thread &operator=(const Thread &other) = delete;
  Thread &operator=(Thread &&other) = delete;

 private:
  void Make();

  auto IsNeedWhile() const -> bool;

  void ExitThread();

  static void MyThreadFunction(void *lpParam);

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
