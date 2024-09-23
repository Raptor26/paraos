/// @file paraos_queue.hpp
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

#ifndef PARAOS_QUEUE_HPP
#define PARAOS_QUEUE_HPP

#include <cassert>
#include <memory>
#include <optional>

#include "etl/queue.h"
#include "gsl/gsl"
#include "paraos_config.hpp"
#include "paraos_mutex.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_trace.hpp"

namespace paraos {

template <typename T, const size_t SIZE>
struct Queue {
 private:
  static_assert(
      std::is_nothrow_move_constructible<T>::value,
      "'T' move constructor must be annotated as noexcept");

  static_assert(SIZE > 0, "Queue size must be greater then '0'");

 public:
  Queue() {}

  virtual ~Queue() { Erase(); };

  Queue(const Queue& other) = delete;
  Queue(Queue&& other) = delete;
  Queue& operator=(const Queue& other) = delete;
  Queue& operator=(Queue&& other) = delete;

  operator bool() const { return true; }

  template <typename... Args>
  auto EmplaceBack(Args&&... args) -> bool {
    bool is_pushed{false};
    if (!queue_.full()) {
      queue_.emplace(std::forward<Args>(args)...);

      is_pushed = true;
    }
    return is_pushed;
  }

  auto Push(T&& item) noexcept(std::is_nothrow_move_constructible<T>::value)
      -> bool {
    bool is_pushed{false};
    if (!queue_.full()) {
      queue_.push(std::move(item));
      is_pushed = true;
    }

    return is_pushed;
  }

  auto Push(const T& item) noexcept(
      std::is_nothrow_copy_constructible<T>::value) -> bool {
    bool is_pushed{false};
    if (!queue_.full()) {
      queue_.push(item);
      is_pushed = true;
    }

    return is_pushed;
  }

  auto Pop() -> std::optional<T> {
    if (IsEmpty()) {
      return std::nullopt;
    }

    // Лямбда-функция ниже будет вызвана сразу после оператора return
    auto pop_from_queue = gsl::finally([&] {
      paraosTRACE_MESSAGE("Call pop() for queue");

      queue_.pop();
    });
    // После оператора return будет вызвана лямбда-функция выше, которая
    // освободит память в очереди
    return std::move(queue_.front());
  }

  PARAOS_INLINE_TRIVIAL auto IsEmpty() -> bool { return queue_.empty(); };

  PARAOS_INLINE_TRIVIAL auto IsFull() -> bool { return queue_.full(); }

  PARAOS_INLINE_TRIVIAL auto Size() -> size_t { return queue_.size(); };

  PARAOS_INLINE_OPERATIONS void Erase() { queue_.clear(); };

 protected:
  auto IsQueueReady() const -> bool { return *this; }

 private:
  /// @brief
  etl::queue<T, SIZE> queue_;
};

}  // namespace paraos

#endif /* PARAOS_QUEUE_HPP */
