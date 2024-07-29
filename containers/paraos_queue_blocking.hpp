#ifndef PARAOS_QUEUE_BLOCKING_HPP
#define PARAOS_QUEUE_BLOCKING_HPP

#include "paraos_config.hpp"
#include "paraos_critical.hpp"
#include "paraos_queue.hpp"

namespace paraos {

template <typename T>
struct IQueueBlocking {
  virtual ~IQueueBlocking() = default;

  virtual auto Push(const T& element, std::size_t timeout_ms) -> bool = 0;
  virtual auto Push(T&& element, std::size_t timeout_ms) -> bool = 0;
  virtual auto Pop(std::size_t timeout_ms) -> T = 0;
  virtual auto IsEmpty() -> bool = 0;
  virtual auto IsFull() -> bool = 0;
  virtual auto Size() -> size_t = 0;
  virtual void Erase() = 0;

 protected:
  IQueueBlocking() = default;
};

template <typename T, typename ALLOCATOR = std::allocator<T>>
class QueueBlocking final : public Queue<T, ALLOCATOR>,
                            public IQueueBlocking<T> {
 public:
  QueueBlocking(size_t max_elements_numb)
      : Queue<T, ALLOCATOR>{max_elements_numb},
        push_sem_{SemaphoreAttr{max_elements_numb}},
        pop_sem_{SemaphoreAttr{max_elements_numb}} {}

  virtual ~QueueBlocking() {}

  operator bool() const {
    bool queue_ready{false};

    if (push_sem_ && pop_sem_ && Queue<T, ALLOCATOR>::IsQueueReady()) {
      queue_ready = true;
    }

    return queue_ready;
  }

  template <typename... Args>
  auto EmplaceBack(Args&&... args) -> bool {
    bool is_pushed =
        Queue<T, ALLOCATOR>::EmplaceBack(std::forward<Args>(args)...);

    if (is_pushed) {
      paraosTRACE_MESSAGE("BlockingQueue giving PUSH semaphore");

      push_sem_.Give();
    }

    return is_pushed;
  }

  auto Push(T&& item, std::size_t timeout_ms) noexcept(
      noexcept(QueueBlocking<T, ALLOCATOR>::EmplaceBack(std::move(item))))
      -> bool override {
    if (Queue<T, ALLOCATOR>::IsFull()) {
      paraosTRACE_MESSAGE("BlockingQueue full, POP semaphore waiting...");

      if (pop_sem_.Take(timeout_ms)) {
        paraosTRACE_MESSAGE("BlockingQueue POP semaphore taken, pushing...");

        return QueueBlocking<T, ALLOCATOR>::EmplaceBack(std::move(item));
      }
    } else {
      paraosTRACE_MESSAGE("BlockingQueue not full, pushing...");

      return QueueBlocking<T, ALLOCATOR>::EmplaceBack(std::move(item));
    }
    return false;
  }

  auto Push(const T& item, std::size_t timeout_ms) noexcept(
      noexcept(Queue<T, ALLOCATOR>::Push(item))) -> bool override {
    bool is_pushed{false};

    if (IsFull()) {
      paraosTRACE_MESSAGE("BlockingQueue full, POP semaphore waiting...");

      if (pop_sem_.Take(timeout_ms)) {
        paraosTRACE_MESSAGE("BlockingQueue POP semaphore taken, pushing...");

        is_pushed = Queue<T, ALLOCATOR>::Push(item);
      }
    } else {
      paraosTRACE_MESSAGE("BlockingQueue not full, pushing...");

      is_pushed = Queue<T, ALLOCATOR>::Push(item);
    }

    if (is_pushed) {
      paraosTRACE_MESSAGE("BlockingQueue giving PUSH semaphore");

      push_sem_.Give();
    }

    return is_pushed;
  }

  auto Pop(std::size_t timeout_ms) noexcept(
      noexcept(Queue<T, ALLOCATOR>::Pop())) -> T override {
    paraosTRACE_MESSAGE("BlockingQueue taking PUSH semaphore");

    if (push_sem_.Take(timeout_ms)) {
      paraosTRACE_MESSAGE("BlockingQueue PUSH semaphore taken successfully");

      auto popped_value = Queue<T, ALLOCATOR>::Pop();
      pop_sem_.Give();

      paraosTRACE_MESSAGE("BlockingQueue giving POP semaphore");

      return popped_value;
    } else {
      paraosTRACE_MESSAGE(
          "BlockingQueue PUSH semaphore take failed returning default "
          "object...");

      return T{};
    }
  }

  PARAOS_INLINE_TRIVIAL void Erase() override {
    const paraos::CriticalSection critical;
    Queue<T, ALLOCATOR>::Erase();
  }

  PARAOS_INLINE_TRIVIAL auto IsEmpty() -> bool override {
    const paraos::CriticalSection critical;
    return Queue<T, ALLOCATOR>::IsEmpty();
  }

  PARAOS_INLINE_TRIVIAL auto IsFull() -> bool override {
    const paraos::CriticalSection critical;
    return Queue<T, ALLOCATOR>::IsFull();
  }

  virtual auto Size() -> size_t override {
    const paraos::CriticalSection critical;
    return Queue<T, ALLOCATOR>::Size();
  }

 private:
  Semaphore push_sem_;
  Semaphore pop_sem_;
};

}  // namespace paraos

#endif /* PARAOS_QUEUE_BLOCKING_HPP */
