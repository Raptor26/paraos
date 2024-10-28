#ifndef PARAOS_QUEUE_LOCKING_V3_HPP
#define PARAOS_QUEUE_LOCKING_V3_HPP

#include <execution>
#include <optional>
#include <utility>
#include <vector>

#include "etl/queue.h"
#include "paraos_config.hpp"
#include "paraos_critical.hpp"
#include "paraos_mutex.hpp"
#include "paraos_mutex_raii.hpp"
#include "paraos_runtime_profiler.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_thread.hpp"

namespace paraos {
namespace v3 {

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
  auto TryEmplaceBack(bool is_isr, Args&&... args) -> bool {
    bool is_pushed{false};

    try {
      const paraos::CriticalSection critical;
      queue_.emplace(std::forward<Args>(args)...);
      pop_sem_.Give(is_isr);
      is_pushed = true;
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
  auto TryPush(T&& item, bool is_isr = false) -> bool {
    return TryEmplaceBack(is_isr, std::move(item));
  }

  /// @brief Try push copy item in queue. If queue full, nothing will push.
  ///
  /// @param[in] item: rvalue item for move in queue.
  /// @param[in] is_isr: Set true if TryPush() calls from isr.
  ///
  /// @return Return true if item successfully copied in queue. false in other
  /// wise.
  auto TryPush(const T& item, bool is_isr = false) -> bool {
    bool is_pushed{false};

    try {
      const paraos::CriticalSection critical;
      queue_.push(item);
      pop_sem_.Give(is_isr);
      is_pushed = true;
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

  /// @brief Moved object from queue and pop queue.
  /// @param[in] timeout_ms: Timeout for waiting when item is queue will be
  /// available for read.
  /// @return Read object, contained in std::optional. If no object read,
  /// std::optional not contained any value.
  auto Pop(std::size_t timeout_ms, bool is_isr = false) -> std::optional<T> {
    std::optional<T> optional;

    MutexGuard mutex(mutex_);

    bool is_need_take{true};

    auto timeout = Thread::GetCurrentTime();

    while (IsEmpty()) {
      if (pop_sem_.Take(timeout_ms, is_isr)) {
        paraosTRACE_MESSAGE("Sem taken");
        break;
      } else {
        paraosTRACE_MESSAGE("Sem not taken!!!");

        // recalculate timeout. timeout_ms value will corrected in
        // CheckTimeout().
        if (Thread::CheckTimeout(timeout, timeout_ms)) {
          is_need_take = false;
          break;
        }
      }
    }

    if (is_need_take) {
      try {
        const paraos::CriticalSection critical;
        paraosTRACE_MESSAGE("Try pop form queue");
        optional.emplace(std::move(queue_.front()));
        queue_.pop();
        paraosTRACE_MESSAGE("Queue Pop success");
      } catch (const etl::queue_empty e) {
        optional.reset();
      }
    }

    return optional;
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
  IQueueBlocking(etl::iqueue<T>& queue, SemaphoreBinary& pop_sem, Mutex& mutex)
      : queue_{queue}, pop_sem_{pop_sem}, mutex_{mutex} {}

 private:
  etl::iqueue<T>& queue_;
  SemaphoreBinary& pop_sem_;
  Mutex& mutex_;
};

template <typename T, const std::size_t SIZE>
class QueueBlocking final : public IQueueBlocking<T> {
  static_assert(SIZE > 1u, "Queue size must be greater then 1 item");

 public:
  QueueBlocking()
      : IQueueBlocking<T>{queue_, pop_sem_, mutex_},
        pop_sem_{SemaphoreAttr{SIZE, SIZE}} {}

  virtual ~QueueBlocking() = default;

  operator bool() const {
    bool queue_ready{false};

    if (pop_sem_ && (queue_.capacity() == SIZE)) {
      queue_ready = true;
    }

    return queue_ready;
  }

 private:
  etl::queue<T, SIZE> queue_;
  SemaphoreBinary pop_sem_;
  Mutex mutex_;
};
}  // namespace v3
}  // namespace paraos

#endif /* PARAOS_QUEUE_LOCKING_V3_HPP */
