#ifndef PARAOS_SEMAPHORE_HPP
#define PARAOS_SEMAPHORE_HPP

#include <stddef.h>

#include "FreeRTOS.h"
#include "paraos_check.h"
#include "paraos_utils.hpp"
#include "semphr.h"

namespace paraos {

/// @brief Аттрибуты семафора, передаваемые ему при инициализации.
struct SemaphoreAttr {
  std::size_t max_count = 1u;
};

constexpr std::size_t initial_count = 0u;

/// @brief Класс-реализация семафоров freeRTOS.
class Semaphore {
 public:
  /// @brief Конструктор семафора.
  /// @param[in] attr: Аттрибуты семафора.
  Semaphore(const SemaphoreAttr attr) noexcept {
#ifdef paraosTRACE_ENABLE
    std::cout << "Semaphore Ctor" << std::endl;
#endif
    // В зависимости от максимального значения счётчика создаваемого семафора
    // будет создан либо бинарный, либо счётный семафор.
    if (attr.max_count < 2) {
      handle_ = xSemaphoreCreateBinary();
    } else {
      handle_ = xSemaphoreCreateCounting(attr.max_count, initial_count);
    }
  }

  virtual ~Semaphore() {
    if (handle_) {
      vSemaphoreDelete(handle_);

      // need for debug only
      handle_ = nullptr;
    }

#ifdef paraosTRACE_ENABLE
    std::cout << "Semaphore Dtor" << std::endl;
#endif
  }

  operator bool() const { return handle_ != nullptr ? true : false; }

  /// @brief Метод уменьшает счётчик семафора на 1, если значение счётчика равно
  /// 0, произойдёт блокировка вызывающего потока на указанное время, либо, пока
  /// другой поток не увеличит счётчик.
  /// @param[in] timeout_ms: Время ожидания счётчика семафора в мс.
  /// @return Возвращает результат ожидания счётчика семафора.
  bool Take(std::size_t timeout_ms = max_delay) {
    PARAOS_CHECK_ASSERT(handle_ != nullptr);
    return static_cast<bool>(
        xSemaphoreTake(handle_, RTOS_THREAD_ConvertMsToTicks(timeout_ms)));
  }

  /// @brief Метод увеличивает значение счётчика на 1.
  /// @param[in] from_isr: Флаг вызова метода более приоритетным потоком.
  /// @return Возвращает результат выполнения операции.
  bool Give(bool from_isr = false) {
    PARAOS_CHECK_ASSERT(handle_ != nullptr);

    auto success = pdTRUE;
    if (from_isr == true) {
      BaseType_t higher_priority_task_woken = 1;
      success = xSemaphoreGiveFromISR(handle_, &higher_priority_task_woken);
    } else {
      success = xSemaphoreGive(handle_);
    }

    return success == pdTRUE ? true : false;
  }

 protected:
  Semaphore() : Semaphore{SemaphoreAttr{}} {}

 private:
  SemaphoreHandle_t handle_{nullptr};
};

/// @brief Класс-реализация бинарного семафора.
struct SemaphoreBinary final : public Semaphore {
  /// @brief Конструктор по умолчанию для бинарного семафора.
  SemaphoreBinary() noexcept : Semaphore{} {}

  ~SemaphoreBinary() = default;
};
}  // namespace paraos

#endif /* PARAOS_SEMAPHORE_HPP */
