#ifndef PARAOS_MUTEX_HPP
#define PARAOS_MUTEX_HPP

#include <stddef.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "paraos_bool_atomic.hpp"
#include "paraos_check.h"
#include "paraos_critical.hpp"
#include "paraos_utils.hpp"
#include "semphr.h"

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

namespace paraos {

/// @brief Атрибуты мьютекса, используемые при его создании.
struct MutexAttr {
  bool is_binary_ = false;
};

/// @brief Класс-реализация мьютексов в freeRTOS, общий для всех разновидностей
/// мьютексов.
class MutexBase {
 public:
  /// @brief Конструктор MutexBase.
  /// @param[in] attr: Атрибуты мьютекса.
  MutexBase(const MutexAttr& attr);

  /// @brief Конструктор MutexBase по умолчанию.
  MutexBase();

  virtual ~MutexBase();

  MutexBase(const MutexBase& other) = delete;
  MutexBase(MutexBase&& other) = delete;

  MutexBase& operator=(const MutexBase& other) = delete;
  MutexBase& operator=(MutexBase&& other) = delete;

  operator bool() const;

  /// @brief Метод блокирует вызывающий поток до тех пор, пока
  /// этот поток не получит права владения мьютексом.
  /// @param[in] timeout_ms: Время ожидания получения права владения мьютексом в
  /// мс.
  /// @return Возвращает результат ожидания получения права владения мьютексом.
  virtual bool Lock(std::size_t timeout_ms = max_delay);

  /// @brief Метод выпускает права владения мьютексом из вызывающего потока.
  /// @return Возвращает результат операции выпуска прав владения мьютексом.
  virtual bool Unlock();

 private:
  SemaphoreHandle_t handle_{nullptr};
};

/// @brief Класс-реализация бинарного мьютекса.
class MutexBaseBinary : public MutexBase {
 public:
  /// @brief Конструктор по умолчанию.
  MutexBaseBinary();

  virtual ~MutexBaseBinary();

  MutexBaseBinary(const MutexBaseBinary& other) = delete;
  MutexBaseBinary(MutexBaseBinary&& other) = delete;

  MutexBaseBinary& operator=(const MutexBaseBinary& other) = delete;
  MutexBaseBinary& operator=(MutexBaseBinary&& other) = delete;

  /// @brief Метод блокирует вызывающий поток до тех пор, пока
  /// этот поток не получит права владения мьютексом.
  /// @param[in] timeout_ms: Время ожидания получения права владения мьютексом в
  /// мс.
  /// @return Возвращает результат ожидания получения права владения мьютексом.
  virtual bool Lock(std::size_t timeout_ms = max_delay) override;

  /// @brief Метод выпускает права владения мьютексом из вызывающего потока.
  /// @return Возвращает результат операции выпуска прав владения мьютексом.
  virtual bool Unlock() override;

 private:
  /// @brief Safe thread flag
  BoolAtomic is_locked_{false};
};

/// @brief Класс рекурсивного мьютекса.
/// @note Рекурсивный мьютекс позволяет вызывать метод Lock() более 1 раза без
/// вызова Unlock().
class RecursiveMutex {
 public:
  /// @brief Конструктор класса рекурсивного мьютекса.
  /// @param[in] attr: Атрибуты мьютекса.
  RecursiveMutex(const MutexAttr& attr);

  RecursiveMutex();

  virtual ~RecursiveMutex();

  RecursiveMutex(const RecursiveMutex& other) = delete;
  RecursiveMutex(RecursiveMutex&& other) = delete;

  RecursiveMutex& operator=(const RecursiveMutex& other) = delete;
  RecursiveMutex& operator=(RecursiveMutex&& other) = delete;

  operator bool() const;

  /// @brief Метод блокирует вызывающий поток до тех пор, пока
  /// этот поток не получит права владения мьютексом.
  /// @param[in] timeout_ms: Время ожидания получения права владения мьютексом в
  /// мс.
  /// @return Возвращает результат ожидания получения права владения мьютексом.
  virtual bool Lock(std::size_t timeout_ms = max_delay);

  /// @brief Метод выпускает права владения мьютексом из вызывающего потока.
  /// @return Возвращает результат операции выпуска прав владения мьютексом.
  virtual bool Unlock();

 private:
  SemaphoreHandle_t handle_{nullptr};
};

}  // namespace paraos

#endif /* PARAOS_MUTEX_HPP */
