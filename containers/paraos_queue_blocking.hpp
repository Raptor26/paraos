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

  /// @brief
  ///
  /// @note `PARAOS_ATTR_UNUSED Args&&... args` suppress warning: unused
  /// parameter 'args' [-Werror,-Wunused-parameter] [build] 87 | auto
  /// EmplaceBack(Args&&... args) -> bool {
  ///
  /// @tparam ...Args
  /// @param ...args
  /// @return
  template <typename... Args>
  auto EmplaceBack(PARAOS_ATTR_UNUSED Args&&... args) -> bool {
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
        is_pushed = queue_.Push(std::move(item));
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
        is_pushed = queue_.Push(item);
      }

      push_sem_.Give();
    }

    return is_pushed;
  }

  auto Pop(std::size_t timeout_ms) -> std::optional<T> {
    paraosTRACE_MESSAGE("BlockingQueue taking PUSH semaphore");

    if (push_sem_.Take(timeout_ms)) {
      // Лямбда-функция ниже будет вызвана сразу после оператора return
      auto pop_from_queue = gsl::finally([&] { pop_sem_.Give(); });

      const paraos::CriticalSection critical;
      paraosTRACE_MESSAGE("BlockingQueue PUSH semaphore taken successfully");
      return queue_.Pop();
    }

    return std::nullopt;
  }

  PARAOS_INLINE_TRIVIAL void Erase() {
    const paraos::CriticalSection critical;
    queue_.Erase();
  }

  PARAOS_INLINE_TRIVIAL auto IsEmpty() -> bool {
    const paraos::CriticalSection critical;
    return queue_.IsEmpty();
  }

  PARAOS_INLINE_TRIVIAL auto IsFull() -> bool {
    const paraos::CriticalSection critical;
    return queue_.IsFull();
  }

  virtual auto Size() -> size_t {
    const paraos::CriticalSection critical;
    return queue_.Size();
  }

 protected:
  IQueueBlocking(
      paraos::IQueue<T>& queue, SemaphoreCounting& push_sem,
      SemaphoreCounting& pop_sem)
      : queue_{queue}, push_sem_{push_sem}, pop_sem_{pop_sem} {}

 private:
  paraos::IQueue<T>& queue_;
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

    if (push_sem_ && pop_sem_ && queue_.IsQueueReady()) {
      queue_ready = true;
    }

    return queue_ready;
  }

 private:
  paraos::Queue<T, SIZE> queue_;
  SemaphoreCounting push_sem_;
  SemaphoreCounting pop_sem_;
};

}  // namespace paraos

#endif /* PARAOS_QUEUE_BLOCKING_HPP */
