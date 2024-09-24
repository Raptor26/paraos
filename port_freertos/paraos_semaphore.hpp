/// @file paraos_semaphore.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author VyhodcevEgor <vyhodcev@internet.ru>
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

#ifndef PARAOS_SEMAPHORE_HPP
#define PARAOS_SEMAPHORE_HPP

#include <stddef.h>

#include "FreeRTOS.h"
#include "paraos_utils.hpp"
#include "paroas_isr.hpp"
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
  Semaphore(const SemaphoreAttr attr) noexcept;

  virtual ~Semaphore();

  operator bool() const;

  /// @brief Метод уменьшает счётчик семафора на 1, если значение счётчика равно
  /// 0, произойдёт блокировка вызывающего потока на указанное время, либо, пока
  /// другой поток не увеличит счётчик.
  /// @param[in] timeout_ms: Время ожидания счётчика семафора в мс.
  /// @return Возвращает результат ожидания счётчика семафора.
  ISRbool Take(std::size_t timeout_ms = max_delay, bool from_isr = false);

  /// @brief Release semaphore.
  /// @param[in] from_isr: Set true, if called from isr.
  /// @return Return operation status. ISRbool contained value indicate is need
  /// switch context. Useful when Give() called from isr.
  ISRbool Give(bool from_isr = false);

 protected:
  Semaphore() : Semaphore{SemaphoreAttr{}} {}

 private:
  SemaphoreHandle_t handle_{nullptr};

  bool is_recursive_;
};

/// @brief Класс-реализация бинарного семафора.
struct SemaphoreBinary final : public Semaphore {
  /// @brief Конструктор по умолчанию для бинарного семафора.
  SemaphoreBinary() noexcept : Semaphore{} {}

  ~SemaphoreBinary() = default;
};
}  // namespace paraos

#endif /* PARAOS_SEMAPHORE_HPP */
