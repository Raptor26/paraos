/// @file paraos_queue_blocking.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_QUEUE_BLOCKING_HPP
#define PARAOS_QUEUE_BLOCKING_HPP

#include <chrono>
#include <execution>
#include <mutex>
#include <optional>
#include <utility>

#include "etl/queue.h"
#include "paraos_check.h"
#include "paraos_config.hpp"
#include "paraos_critical.hpp"
#include "paraos_isr.hpp"
#include "paraos_mutex_std.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_semaphore_std.hpp"
#include "paraos_time.hpp"
#include "paraos_trace.hpp"

namespace paraos {

template <typename T, const std::size_t SIZE>
class IQueueBlocking {
 public:
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

#ifndef ETL_CHECK_PUSH_POP
#error "try/catch section below needs to ETL_CHECK_PUSH_POP definition"
#endif
    try {
      const paraos::CriticalSection critical{is_isr};
      queue_.emplace(std::forward<Args>(args)...);

      // Assignment here is needed for updating "is_need_switch_context_" state.
      pop_sem_.release();

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
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    auto start_time = paraos::GetCurrentTime();
    std::unique_lock<paraos::mutex> lock(mutex_);

    // Always acquire the semaphore before popping. This keeps the semaphore
    // counter in sync with the number of items in the queue even when multiple
    // producers have woken the queue before a consumer finishes popping.
    //
    // The mutex is released while waiting so that producers can still push
    // items; holding it during the whole wait would let the semaphore count
    // grow past the queue size while a consumer has taken a notification but
    // has not popped yet.
    if (IsEmpty()) {
      lock.unlock();
      while (true) {
        if (pop_sem_.try_acquire_for(std::chrono::milliseconds(timeout_ms))) {
          paraosTRACE_MESSAGE("Sem taken");
          break;
        }

        if (paraos::CheckTimeout(start_time, timeout_ms)) {
          paraosTRACE_MESSAGE("Timeout expired");
          return std::nullopt;
        }
      }
      lock.lock();
    } else if (!pop_sem_.try_acquire()) {
      // Another consumer took the notification while we were locking; wait
      // for the next one without holding the mutex.
      lock.unlock();
      while (true) {
        if (pop_sem_.try_acquire_for(std::chrono::milliseconds(timeout_ms))) {
          paraosTRACE_MESSAGE("Sem taken");
          break;
        }

        if (paraos::CheckTimeout(start_time, timeout_ms)) {
          paraosTRACE_MESSAGE("Timeout expired");
          return std::nullopt;
        }
      }
      lock.lock();
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
  IQueueBlocking(
      etl::iqueue<T>& queue,
      paraos::counting_semaphore<static_cast<std::ptrdiff_t>(SIZE)>& pop_sem,
      paraos::mutex& mutex)
      : queue_{queue}, pop_sem_{pop_sem}, mutex_{mutex} {}

 private:
  etl::iqueue<T>& queue_;
  paraos::counting_semaphore<static_cast<std::ptrdiff_t>(SIZE)>& pop_sem_;
  paraos::mutex& mutex_;
};

template <typename T, const std::size_t SIZE>
class QueueBlocking final : public IQueueBlocking<T, SIZE> {
  static_assert(SIZE > 1U, "Queue size must be greater then 1 item");

 public:
  QueueBlocking() noexcept
      : IQueueBlocking<T, SIZE>{queue_, pop_sem_, mutex_} {}

  ~QueueBlocking() override = default;

  explicit operator bool() const {
    bool queue_ready{false};

    if (queue_.capacity() == SIZE) {
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
  paraos::counting_semaphore<static_cast<std::ptrdiff_t>(SIZE)> pop_sem_{0};
  paraos::mutex mutex_;
};
}  // namespace paraos

#endif /* PARAOS_QUEUE_BLOCKING_HPP */
