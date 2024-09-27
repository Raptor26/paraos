/// @file paraos_queue_blocking.hpp
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

#ifndef PARAOS_QUEUE_BLOCKING_HPP
#define PARAOS_QUEUE_BLOCKING_HPP

#include "gsl/gsl"
#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_config.hpp"
#include "paraos_critical.hpp"
#include "paraos_mutex.hpp"
#include "paraos_mutex_raii.hpp"
#include "paraos_queue.hpp"

namespace paraos {

template <typename T>
struct IQueueBlocking {
  virtual ~IQueueBlocking() = default;

  /// @brief Construct object "in place" in queue storage.
  ///
  /// @tparam Args: Arguments to be passed in ctor for construct object in
  /// place.
  /// @param[in] args: Arguments to be passed in ctor for construct object in
  /// place.
  /// @return true if object constructed, false otherwise.
  template <typename... Args>
  auto EmplaceBack(Args&&... args) -> bool {
    bool is_pushed{false};
    if (pop_sem_.Take(0u)) {
      if (!queue_.IsFull()) {
        is_pushed = queue_.EmplaceBack(std::forward<Args>(args)...);
      }
      push_sem_.Give();
    }
    return is_pushed;
  }

  auto Push(T&& item, std::size_t timeout_ms) -> bool {
    paraosTRACE_MESSAGE("BlockingQueue full, POP semaphore waiting...");

    bool is_pushed{false};

    if (pop_sem_.Take(timeout_ms)) {
      {
        const paraos::CriticalSection critical;
        paraosTRACE_MESSAGE("BlockingQueue POP semaphore taken, pushing...");

        try {
          queue_.push(std::move(item));
          is_pushed = true;
        } catch (etl::queue_full& e) {
          paraosTRACE_MESSAGE(e.file_name() << e.line_number() << e.what());
        }
      }

      push_sem_.Give();
    }

    return is_pushed;
  }

  auto Push(const T& item, std::size_t timeout_ms) -> bool {
    bool is_pushed{false};

    if (pop_sem_.Take(timeout_ms)) {
      {
        const paraos::CriticalSection critical;
        paraosTRACE_MESSAGE("BlockingQueue POP semaphore taken, pushing...");

        try {
          queue_.push(item);
          is_pushed = true;
        } catch (etl::queue_full& e) {
          paraosTRACE_MESSAGE(e.file_name() << e.line_number() << e.what());
        }
      }

      push_sem_.Give();
    }

    return is_pushed;
  }

  /// @brief Moved object from queue and pop queue.
  /// @param[in] timeout_ms: Timeout for waiting when item is queue will be
  /// available for read.
  /// @return Read object, contained in std::optional. If no object read,
  /// std::optional not contained any value.
  auto Pop(std::size_t timeout_ms) -> std::optional<T> {
    paraosTRACE_MESSAGE("BlockingQueue taking PUSH semaphore");

    if (push_sem_.Take(timeout_ms)) {
      // pop_sem_.Give() will be called after return.
      auto sem_give = gsl::finally([&] { pop_sem_.Give(); });

      // Moved value from queue in std::optional<T>.
      return FrontAndPop();
    }

    return std::nullopt;
  }

  PARAOS_INLINE_TRIVIAL void Erase() {
    const paraos::CriticalSection critical;
    queue_.clear();
  }

  PARAOS_INLINE_TRIVIAL auto IsEmpty() -> bool {
    const paraos::CriticalSection critical;
    return queue_.empty();
  }

  PARAOS_INLINE_TRIVIAL auto IsFull() -> bool {
    const paraos::CriticalSection critical;
    return queue_.full();
  }

  PARAOS_INLINE_TRIVIAL auto Size() -> size_t {
    const paraos::CriticalSection critical;
    return queue_.size();
  }

 protected:
  IQueueBlocking(
      etl::iqueue<T>& queue, SemaphoreCounting& push_sem,
      SemaphoreCounting& pop_sem)
      : queue_{queue}, push_sem_{push_sem}, pop_sem_{pop_sem} {}

 private:
  auto FrontAndPop() -> std::optional<T> {
    const paraos::CriticalSection critical;

    // When FrontAndPop() called in Pop(), semaphore contained information about
    // items numb in queue. In this case, we don't need check is queue empty.
    if (!queue_.empty()) {
      // Lambda below will called after return operator.
      auto pop_from_queue = gsl::finally([&] {
        // We check is queue empty above, exertion can't be throw.
        queue_.pop();
      });

      // Move object from queue, then, after return, lambda above delete object
      // from queue with pop() operation.
      return std::move(queue_.front());
    }

    return std::nullopt;
  }

 private:
  etl::iqueue<T>& queue_;
  SemaphoreCounting& push_sem_;
  SemaphoreCounting& pop_sem_;
};

template <typename T, const std::size_t SIZE>
class QueueBlocking final : public IQueueBlocking<T> {
 public:
  QueueBlocking()
      : IQueueBlocking<T>{queue_, push_sem_, pop_sem_},
        push_sem_{SemaphoreAttr{SIZE}},
        pop_sem_{SemaphoreAttr{SIZE}} {
    for (std::size_t i = 0u; i < SIZE; ++i) {
      // необходимо отдать семафор pop_sem_ столько раз, сколько элементов может
      // хранить очередь. Иначе при вызове Push() семафор не будет получен
      // никогда.
      pop_sem_.Give();
      push_sem_.Take(0u);
    }
  }

  virtual ~QueueBlocking() {}

  operator bool() const {
    bool queue_ready{false};

    if (push_sem_ && pop_sem_ && (queue_.capacity() == SIZE)) {
      queue_ready = true;
    }

    return queue_ready;
  }

 private:
  etl::queue<T, SIZE> queue_;
  SemaphoreCounting push_sem_;
  SemaphoreCounting pop_sem_;
};

}  // namespace paraos

#endif /* PARAOS_QUEUE_BLOCKING_HPP */
