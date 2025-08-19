/// @file paraos_queue_blocking.hpp
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

#ifndef PARAOS_QUEUE_BLOCKING_HPP
#define PARAOS_QUEUE_BLOCKING_HPP

#include <execution>
#include <optional>
#include <utility>

#include "etl/queue.h"
#include "paraos_check.h"
#include "paraos_config.hpp"
#include "paraos_critical.hpp"
#include "paraos_isr.hpp"
#include "paraos_mutex.hpp"
#include "paraos_mutex_raii.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_time.hpp"

namespace paraos {

template <typename T>
struct IQueueBlocking {
  virtual ~IQueueBlocking() = default;

  /// @brief Construct object "in place" in queue storage.
  ///
  /// @note 'is_isr' set as first parameter because argument pack (args) must be
  /// last in argument list.
  ///
  /// @tparam Args: Arguments to be passed in ctor for construct object in
  /// place.
  /// @param[in] args: Arguments to be passed in ctor for construct object in
  /// place.
  ///
  /// @return true if object constructed, false otherwise.
  template <typename... Args>
  auto TryEmplaceBack(bool is_isr, Args&&... args) noexcept -> paraos::ISRbool {
    paraos::ISRbool is_pushed;

#if !defined(ETL_CHECK_PUSH_POP)
#error "try/catch section below needs to ETL_CHECK_PUSH_POP definition"
#endif
    try {
      const paraos::CriticalSection critical{is_isr};
      queue_.emplace(std::forward<Args>(args)...);

      // Assignment here is needed for updating "is_need_switch_context_" state.
      is_pushed = pop_sem_.Give(is_isr);

      // Semaphore always given successful.
      is_pushed.SetSuccessStatus(true);
    } catch (const etl::queue_full& e) {
      // queue full. Nothing push in queue. In IQueueBlocking API it's not
      // problem. TryPush() return false.
    } catch (const std::exception& e) {
      // moved object throw exception. Best what we can in this case - print
      // debug message.
      paraosTRACE_MESSAGE(e.what());
    }

    return is_pushed;
  }

  /// @brief Try move item in queue. If queue full, nothing will move.
  ///
  /// @param[in] item: lvalue item for move in queue.
  /// @param[in] is_isr: Set true if TryPush() calls from isr.
  ///
  /// @return Return true if item successfully moved in queue. false in other
  /// wise.
  template <typename U>
  auto TryPush(U&& item, bool is_isr = false) noexcept -> paraos::ISRbool {
    return TryEmplaceBack(is_isr, std::forward<U>(item));
  }

  /// @brief Moved object from queue and pop queue.
  /// @param[in] timeout_ms: Timeout for waiting when item is queue will be
  /// available for read.
  /// @return Read object, contained in std::optional. If no object read,
  /// std::optional not contained any value.
  auto Pop(paraos::delay_type timeout_ms, bool is_isr = false)
      -> std::optional<T> {
    auto start_time = paraos::GetCurrentTime();
    const MutexGuard lock(mutex_);

    if (IsEmpty()) {
      while (true) {
        if (pop_sem_.Take(timeout_ms, is_isr)) {
          paraosTRACE_MESSAGE("Sem taken");
          break;
        }

        if (paraos::CheckTimeout(start_time, timeout_ms)) {
          paraosTRACE_MESSAGE("Timeout expired");
          return std::nullopt;
        }
      }
    }

    const paraos::CriticalSection critical;
    if (!queue_.empty()) {
      paraosTRACE_MESSAGE("Try pop from queue");
      std::optional<T> result = std::move(queue_.front());
      queue_.pop();
      paraosTRACE_MESSAGE("Queue Pop success");
      return result;
    }

    // Если очередь всё-таки пуста — возможно, race или ошибка Push()
    paraosTRACE_MESSAGE(
        "Race condition detected: queue is empty after sem.Give()");
    return std::nullopt;
  }

  PARAOS_INLINE_TRIVIAL void Erase(bool is_isr = false) {
    const paraos::CriticalSection critical{is_isr};
    queue_.clear();
  }

  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto IsEmpty(bool is_isr = false) const
      -> bool {
    const paraos::CriticalSection critical{is_isr};
    return queue_.empty();
  }

  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto IsFull(bool is_isr = false) const
      -> bool {
    const paraos::CriticalSection critical{is_isr};
    return queue_.full();
  }

  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto Size(bool is_isr = false) const
      -> size_t {
    const paraos::CriticalSection critical{is_isr};
    return queue_.size();
  }

  IQueueBlocking(IQueueBlocking&& other) = delete;
  auto operator=(IQueueBlocking&& other) -> IQueueBlocking& = delete;
  auto operator=(const IQueueBlocking& other) -> IQueueBlocking& = delete;
  IQueueBlocking(const IQueueBlocking& other) = delete;

 protected:
  IQueueBlocking(etl::iqueue<T>& queue, SemaphoreBinary& pop_sem, Mutex& mutex)
      : queue_{queue}, pop_sem_{pop_sem}, mutex_{mutex} {}

 private:
  etl::iqueue<T>& queue_;
  SemaphoreBinary& pop_sem_;
  Mutex& mutex_;
};

template <typename T, const std::size_t SIZE>
class QueueBlocking final : public IQueueBlocking<T> {
  static_assert(SIZE > 1U, "Queue size must be greater then 1 item");

 public:
  QueueBlocking() noexcept(std::is_nothrow_constructible<SemaphoreBinary>())
      : IQueueBlocking<T>{queue_, pop_sem_, mutex_},
        pop_sem_{SemaphoreAttr{SIZE, SIZE}} {}

  ~QueueBlocking() override = default;

  explicit operator bool() const {
    bool queue_ready{false};

    if (pop_sem_ && (queue_.capacity() == SIZE)) {
      queue_ready = true;
    }

    return queue_ready;
  }

  /// @brief Five rule.
  QueueBlocking(QueueBlocking&& other) = delete;
  auto operator=(QueueBlocking&& other) -> QueueBlocking& = delete;
  auto operator=(const QueueBlocking& other) -> QueueBlocking& = delete;
  QueueBlocking(const QueueBlocking& other) = delete;

 private:
  etl::queue<T, SIZE> queue_;
  SemaphoreBinary pop_sem_;
  Mutex mutex_;
};
}  // namespace paraos

#endif /* PARAOS_QUEUE_BLOCKING_HPP */
