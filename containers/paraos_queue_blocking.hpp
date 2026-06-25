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
class queue_blocking_base {
 public:
  virtual ~queue_blocking_base() = default;

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
  auto try_emplace_back(bool is_isr, Args&&... args) noexcept -> paraos::isr_bool {
    paraos::isr_bool is_pushed;

#ifndef ETL_CHECK_PUSH_POP
#error "try/catch section below needs to ETL_CHECK_PUSH_POP definition"
#endif
    try {
      const paraos::critical_section critical{is_isr};
      queue_.emplace(std::forward<Args>(args)...);

      // Assignment here is needed for updating "is_need_switch_context_" state.
      pop_sem_.release();

      // Semaphore always given successful.
      is_pushed.set_success_status(true);
    } catch (const etl::queue_full& e) {
      // queue full. Nothing push in queue. In queue_blocking_base API it's not
      // problem. try_emplace_back() returns false.
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
  /// @param[in] is_isr: Set true if try_push() calls from isr.
  ///
  /// @return Return true if item successfully moved in queue. false in other
  /// wise.
  template <typename U>
  auto try_push(U&& item, bool is_isr = false) noexcept -> paraos::isr_bool {
    return try_emplace_back(is_isr, std::forward<U>(item));
  }

  /// @brief Moved object from queue and pop queue.
  /// @param[in] timeout_ms: Timeout for waiting when item is queue will be
  /// available for read.
  /// @return Read object, contained in std::optional. If no object read,
  /// std::optional not contained any value.
  auto pop(paraos::delay_type timeout_ms, bool is_isr = false)
      -> std::optional<T> {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    auto start_time = paraos::get_current_time();
    std::unique_lock<paraos::mutex> lock(mutex_);

    // Always acquire the semaphore before popping. This keeps the semaphore
    // counter in sync with the number of items in the queue even when multiple
    // producers have woken the queue before a consumer finishes popping.
    //
    // The mutex is released while waiting so that producers can still push
    // items; holding it during the whole wait would let the semaphore count
    // grow past the queue size while a consumer has taken a notification but
    // has not popped yet.
    if (is_empty()) {
      lock.unlock();
      while (true) {
        if (pop_sem_.try_acquire_for(std::chrono::milliseconds(timeout_ms))) {
          paraosTRACE_MESSAGE("Sem taken");
          break;
        }

        if (paraos::check_timeout(start_time, timeout_ms)) {
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

        if (paraos::check_timeout(start_time, timeout_ms)) {
          paraosTRACE_MESSAGE("Timeout expired");
          return std::nullopt;
        }
      }
      lock.lock();
    }

    const paraos::critical_section critical;
    if (!queue_.empty()) {
      paraosTRACE_MESSAGE("Try pop from queue");
      std::optional<T> result = std::move(queue_.front());
      queue_.pop();
      paraosTRACE_MESSAGE("Queue pop success");
      return result;
    }

    // Если очередь всё-таки пуста — возможно, race или ошибка try_push()
    paraosTRACE_MESSAGE(
        "Race condition detected: queue is empty after sem.Give()");
    return std::nullopt;
  }

  PARAOS_INLINE_TRIVIAL void erase(bool is_isr = false) {
    const paraos::critical_section critical{is_isr};
    queue_.clear();
  }

  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto is_empty(bool is_isr = false) const
      -> bool {
    const paraos::critical_section critical{is_isr};
    return queue_.empty();
  }

  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto is_full(bool is_isr = false) const
      -> bool {
    const paraos::critical_section critical{is_isr};
    return queue_.full();
  }

  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto size(bool is_isr = false) const
      -> size_t {
    const paraos::critical_section critical{is_isr};
    return queue_.size();
  }

  // Backward-compatible deprecated forwarding methods.
  template <typename... Args>
  PARAOS_DEPRECATED("use try_emplace_back()")
  auto TryEmplaceBack(bool is_isr, Args&&... args) noexcept -> paraos::isr_bool {
    return try_emplace_back(is_isr, std::forward<Args>(args)...);
  }

  template <typename U>
  PARAOS_DEPRECATED("use try_push()")
  auto TryPush(U&& item, bool is_isr = false) noexcept -> paraos::isr_bool {
    return try_push(std::forward<U>(item), is_isr);
  }

  PARAOS_DEPRECATED("use pop()")
  auto Pop(paraos::delay_type timeout_ms, bool is_isr = false)
      -> std::optional<T> {
    return pop(timeout_ms, is_isr);
  }

  PARAOS_DEPRECATED("use erase()")
  PARAOS_INLINE_TRIVIAL void Erase(bool is_isr = false) { erase(is_isr); }

  [[nodiscard]] PARAOS_DEPRECATED("use is_empty()")
  PARAOS_INLINE_TRIVIAL auto IsEmpty(bool is_isr = false) const -> bool {
    return is_empty(is_isr);
  }

  [[nodiscard]] PARAOS_DEPRECATED("use is_full()")
  PARAOS_INLINE_TRIVIAL auto IsFull(bool is_isr = false) const -> bool {
    return is_full(is_isr);
  }

  [[nodiscard]] PARAOS_DEPRECATED("use size()")
  PARAOS_INLINE_TRIVIAL auto Size(bool is_isr = false) const -> size_t {
    return size(is_isr);
  }

  queue_blocking_base(queue_blocking_base&& other) = delete;
  auto operator=(queue_blocking_base&& other) -> queue_blocking_base& = delete;
  auto operator=(const queue_blocking_base& other) -> queue_blocking_base& = delete;
  queue_blocking_base(const queue_blocking_base& other) = delete;

 protected:
  queue_blocking_base(
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
using IQueueBlocking PARAOS_DEPRECATED("use paraos::queue_blocking_base") = queue_blocking_base<T, SIZE>;

template <typename T, const std::size_t SIZE>
class queue_blocking final : public queue_blocking_base<T, SIZE> {
  static_assert(SIZE > 1U, "Queue size must be greater then 1 item");

 public:
  queue_blocking() noexcept
      : queue_blocking_base<T, SIZE>{queue_, pop_sem_, mutex_} {}

  ~queue_blocking() override = default;

  explicit operator bool() const {
    bool queue_ready{false};

    if (queue_.capacity() == SIZE) {
      queue_ready = true;
    }

    return queue_ready;
  }

  /// @brief Five rule.
  queue_blocking(queue_blocking&& other) = delete;
  auto operator=(queue_blocking&& other) -> queue_blocking& = delete;
  auto operator=(const queue_blocking& other) -> queue_blocking& = delete;
  queue_blocking(const queue_blocking& other) = delete;

 private:
  etl::queue<T, SIZE> queue_;
  paraos::counting_semaphore<static_cast<std::ptrdiff_t>(SIZE)> pop_sem_{0};
  paraos::mutex mutex_;
};

template <typename T, const std::size_t SIZE>
using QueueBlocking PARAOS_DEPRECATED("use paraos::queue_blocking") = queue_blocking<T, SIZE>;
}  // namespace paraos

#endif /* PARAOS_QUEUE_BLOCKING_HPP */
