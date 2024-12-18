/// @file paraos_testing_semaphore.hpp
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
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

/// NAME
///     Модуль, описывающий семафор, который можно использовать при тестировании
///     различных объектов.
///
/// DESCRIPTION
///     В данном файле предоставлена реализация семафора для тестирования
///     различных объектов - вместо блокировки вызывающего потока семафор
///     использует внутренний счётчик, который изменяет своё значение в процессе
///     вызова методов Take() и Give().

#ifndef PARAOS_TESTING_SEMAPHORE_HPP
#define PARAOS_TESTING_SEMAPHORE_HPP

#include <cstdint>

#include "etl/atomic.h"
#include "paraos_attr.h"
#include "paraos_isr.hpp"

namespace paraos {

/// @brief Аттрибуты семафора, передаваемые ему при инициализации.
struct TestingSemaphoreAttr {
  /// @brief Максимальное количество потоков, которые одновременно могут иметь
  /// доступ к участку кода, защищённому семафором.
  size_t max_count{1u};

  /// @brief Начальное значение счётчика семафора.
  size_t initial_count{0u};
};

/// @brief Класс семафора, используемого при тестировании различных объектов.
///
/// @note Его методы являются неблокирующими.
class TestingSemaphore {
 public:
  /// @brief Конструктор класса TestingSemaphore.
  /// @param[in] attrs: Атрибуты, необходимые для инициализации.
  TestingSemaphore(const TestingSemaphoreAttr& attrs)
      : semaphore_counter_{attrs.initial_count}, max_count_{attrs.max_count} {}

  /// @brief Метод выполняет взятие семафора.
  /// @param[in] timeout_ms: Не используется в текущей реализации.
  /// @param[in] from_isr: Не используется в текущей реализации.
  /// @return Возвращает true, если на момент вызова данного метода счётчик
  /// семафора имел значение больше нуля, иначе - false.
  ISRbool Take(std::size_t timeout_ms = 0, bool from_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(timeout_ms);
    PARAOS_ATTR_UNUSED_VAR(from_isr);

    bool take_result{false};

    if (semaphore_counter_ != 0) {
      semaphore_counter_--;
      take_result = true;
    }

    return ISRbool{take_result};
  }

  /// @brief Метод выполняет отдачу семафора.
  /// @param[in] from_isr: Не используется в текущей реализации.
  /// @return Возвращает true, если в момент вызова данного метода значение
  /// счётчика семафора было меньше максимального.
  ISRbool Give(bool from_isr = false) {
    PARAOS_ATTR_UNUSED_VAR(from_isr);

    bool give_result{true};

    ++semaphore_counter_;

    if (semaphore_counter_ > max_count_) {
      semaphore_counter_.store(max_count_);
      give_result = false;
    }

    return ISRbool{give_result};
  }

 private:
  etl::atomic<std::size_t> semaphore_counter_;

  etl::atomic<std::size_t> max_count_;
};

/// @brief Класс бинарного семафора.
///
/// @note Только один поток может выполнять код в участке кода, защищённом
/// бинарным семафором.
class BinaryTestingSemaphore : public TestingSemaphore {
 public:
  /// @brief Конструктор класса бинарного семафора.
  BinaryTestingSemaphore() : BinaryTestingSemaphore{TestingSemaphoreAttr{}} {}

 private:
  BinaryTestingSemaphore(const TestingSemaphoreAttr& attrs)
      : TestingSemaphore{attrs} {}
};

}  // namespace paraos

#endif /* PARAOS_TESTING_SEMAPHORE_HPP */
