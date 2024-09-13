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

extern "C" std::size_t RTOS_THREAD_ConvertMsToTicks(std::size_t uDelayInMs);

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
  MutexBase(const MutexAttr& attr) : handle_{xSemaphoreCreateMutex()} {
    (void)attr;
#ifdef paraosTRACE_ENABLE
    std::cout << "RTOS MutexBase Ctor" << std::endl;
#endif
  }

  /// @brief Конструктор MutexBase по умолчанию.
  MutexBase() : MutexBase(MutexAttr{}) {}

  virtual ~MutexBase() {
    PARAOS_CHECK_ASSERT(handle_ != nullptr);
    if (handle_) {
      vSemaphoreDelete(handle_);

      // need for debug only
      handle_ = nullptr;
    }

#ifdef paraosTRACE_ENABLE
    std::cout << "RTOS MutexBase Dtor" << std::endl;
#endif
  }

  MutexBase(const MutexBase& other) = delete;
  MutexBase(MutexBase&& other) = delete;

  MutexBase& operator=(const MutexBase& other) = delete;
  MutexBase& operator=(MutexBase&& other) = delete;

  operator bool() const { return handle_ != nullptr ? true : false; }

  /// @brief Метод блокирует вызывающий поток до тех пор, пока
  /// этот поток не получит права владения мьютексом.
  /// @param[in] timeout_ms: Время ожидания получения права владения мьютексом в
  /// мс.
  /// @return Возвращает результат ожидания получения права владения мьютексом.
  virtual bool Lock(std::size_t timeout_ms = max_delay) {
    auto result = xSemaphoreTake(
        handle_, (TickType_t)RTOS_THREAD_ConvertMsToTicks(timeout_ms));
    return static_cast<bool>(result);
  }

  /// @brief Метод выпускает права владения мьютексом из вызывающего потока.
  /// @return Возвращает результат операции выпуска прав владения мьютексом.
  virtual bool Unlock() { return static_cast<bool>(xSemaphoreGive(handle_)); }

 private:
  SemaphoreHandle_t handle_{nullptr};
};

/// @brief Класс-реализация бинарного мьютекса.
class MutexBaseBinary : public MutexBase {
 public:
  /// @brief Конструктор по умолчанию.
  MutexBaseBinary() : MutexBase(MutexAttr{true}) {}

  ~MutexBaseBinary() {}

  MutexBaseBinary(const MutexBaseBinary& other) = delete;
  MutexBaseBinary(MutexBaseBinary&& other) = delete;

  MutexBaseBinary& operator=(const MutexBaseBinary& other) = delete;
  MutexBaseBinary& operator=(MutexBaseBinary&& other) = delete;

  /// @brief Метод блокирует вызывающий поток до тех пор, пока
  /// этот поток не получит права владения мьютексом.
  /// @param[in] timeout_ms: Время ожидания получения права владения мьютексом в
  /// мс.
  /// @return Возвращает результат ожидания получения права владения мьютексом.
  virtual bool Lock(std::size_t timeout_ms = max_delay) override {
    bool is_current_operation_locked{false};
    if (is_locked_ == false) {
      is_current_operation_locked = MutexBase::Lock(timeout_ms);
      // We call MutexBase::Lock() if our current state "unlocked". If
      // MutexBase::Lock() returned false (from unlocked state), i don't know
      // what that mean. Try find race condition for "is_locked_" variable in
      // "MutexBaseBinary" class.
      PARAOS_CHECK_ASSERT(is_current_operation_locked == true);
      is_locked_ = true;
    }

    return is_current_operation_locked;
  }

  /// @brief Метод выпускает права владения мьютексом из вызывающего потока.
  /// @return Возвращает результат операции выпуска прав владения мьютексом.
  virtual bool Unlock() override {
    bool is_current_operation_unlocked{false};
    if (is_locked_ == true) {
      is_current_operation_unlocked = MutexBase::Unlock();
      // We call MutexBase::Unlock() if our current state "locked". If
      // MutexBase::Unlock() returned false (from "locked" state), i don't know
      // what that mean. Try find race condition for "is_locked_" variable in
      // "MutexBaseBinary" class.
      PARAOS_CHECK_ASSERT(is_current_operation_unlocked == true);
      is_locked_ = false;
    }

    return is_current_operation_unlocked;
  }

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
  RecursiveMutex(const MutexAttr& attr)
      : handle_{xSemaphoreCreateRecursiveMutex()} {
    (void)attr;
#ifdef paraosTRACE_ENABLE
    std::cout << "RecursiveMutex Ctor" << std::endl;
#endif
  }

  RecursiveMutex() : RecursiveMutex(MutexAttr{}) {}

  virtual ~RecursiveMutex() {
    PARAOS_CHECK_ASSERT(handle_ != nullptr);
    if (handle_) {
      vSemaphoreDelete(handle_);

      // need for debug only
      handle_ = nullptr;
    }

#ifdef paraosTRACE_ENABLE
    std::cout << "RecursiveMutex Dtor" << std::endl;
#endif
  }

  RecursiveMutex(const RecursiveMutex& other) = delete;
  RecursiveMutex(RecursiveMutex&& other) = delete;

  RecursiveMutex& operator=(const RecursiveMutex& other) = delete;
  RecursiveMutex& operator=(RecursiveMutex&& other) = delete;

  operator bool() const { return handle_ != nullptr ? true : false; }

  /// @brief Метод блокирует вызывающий поток до тех пор, пока
  /// этот поток не получит права владения мьютексом.
  /// @param[in] timeout_ms: Время ожидания получения права владения мьютексом в
  /// мс.
  /// @return Возвращает результат ожидания получения права владения мьютексом.
  virtual bool Lock(std::size_t timeout_ms = max_delay) {
    return static_cast<bool>(xSemaphoreTakeRecursive(
        handle_, (TickType_t)RTOS_THREAD_ConvertMsToTicks(timeout_ms)));
  }

  /// @brief Метод выпускает права владения мьютексом из вызывающего потока.
  /// @return Возвращает результат операции выпуска прав владения мьютексом.
  virtual bool Unlock() {
    return static_cast<bool>(xSemaphoreGiveRecursive(handle_));
  }

 private:
  SemaphoreHandle_t handle_{nullptr};
};

}  // namespace paraos

#endif /* PARAOS_MUTEX_HPP */
