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
#include <utility>

#include "etl/atomic.h"
#include "paraos_attr.h"
#include "paraos_isr.hpp"

namespace paraos {

/// @brief Semaphore attributes for it's initialization.
struct TestingSemaphoreAttr {
  /// @brief Maximum number of threads that can execute the code protected by
  /// semaphore at a time.
  size_t max_count{1u};

  /// @brief Semaphore counter initial value.
  size_t initial_count{0u};
};

/// @brief Testing semaphore class.
///
/// @note This class methods are non-blocking.
class TestingSemaphore {
 public:
  /// @brief Testing semaphore constructor.
  /// @param[in] attrs: Attributes for class initialization.
  TestingSemaphore(const TestingSemaphoreAttr &attrs)
      : semaphore_counter_{attrs.initial_count}, max_count_{attrs.max_count} {
    if ((max_count_ > 0) && (semaphore_counter_ <= max_count_)) {
      is_init_succeeded_ = true;
    }
  }

  /// @brief Move ctor.
  TestingSemaphore(TestingSemaphore &&other) {
    if (this != &other) {
      this->is_init_succeeded_ = other.is_init_succeeded_;
      this->semaphore_counter_ = other.semaphore_counter_.load();
      this->max_count_ = other.max_count_.load();
    }
  }

  /// @brief Move assignment.
  TestingSemaphore &operator=(TestingSemaphore &&other) {
    if (this != &other) {
      this->~TestingSemaphore();
      this->is_init_succeeded_ = other.is_init_succeeded_;
      this->semaphore_counter_ = other.semaphore_counter_.load();
      this->max_count_ = other.max_count_.load();
    }

    return *this;
  }

  /// @brief Semaphore non-copyable
  TestingSemaphore(const TestingSemaphore &other) = delete;
  TestingSemaphore &operator=(const TestingSemaphore &other) = delete;

  virtual ~TestingSemaphore() = default;

  /// @brief Take Semaphore.
  ///
  /// @param[in] timeout_ms: Not used in current realization.
  ///
  /// @param[in] from_isr: Not used in current realization.
  ///
  /// @return Return ISRbool with true state if semaphore counter was greater
  /// than zero, false state in otherwise.
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

  /// @brief Release semaphore.
  ///
  /// @param[in] from_isr: Not used in current realization.
  ///
  /// @return Return operation status. ISRbool with true state if semaphore
  /// counter was less than max count, otherwise - false.
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

  operator bool() const { return is_init_succeeded_; }

 protected:
  etl::atomic<std::size_t> semaphore_counter_;

  etl::atomic<std::size_t> max_count_;

  bool is_init_succeeded_{false};
};

/// @brief Binary semaphore class.
///
/// @note Only one thread can execute the code protected with binary semaphore.
class BinaryTestingSemaphore final : public TestingSemaphore {
 public:
  /// @brief Binary semaphore constructor.
  BinaryTestingSemaphore() : BinaryTestingSemaphore{TestingSemaphoreAttr{}} {}

  /// @brief Move ctor.
  BinaryTestingSemaphore(BinaryTestingSemaphore &&other)
      : TestingSemaphore{std::move(other)} {}

  /// @brief Move assignment.
  BinaryTestingSemaphore &operator=(BinaryTestingSemaphore &&other) {
    if (this != &other) {
      this->~BinaryTestingSemaphore();
      this->is_init_succeeded_ = other.is_init_succeeded_;
      this->semaphore_counter_ = other.semaphore_counter_.load();
      this->max_count_ = other.max_count_.load();
    }

    return *this;
  }

  /// @brief Semaphore non-copyable
  BinaryTestingSemaphore(const BinaryTestingSemaphore &other) = delete;
  BinaryTestingSemaphore &operator=(const BinaryTestingSemaphore &other) =
      delete;

  ~BinaryTestingSemaphore() = default;

 private:
  BinaryTestingSemaphore(const TestingSemaphoreAttr &attrs)
      : TestingSemaphore{attrs} {}
};

}  // namespace paraos

#endif /* PARAOS_TESTING_SEMAPHORE_HPP */
