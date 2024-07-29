#ifndef PARAOS_QUEUE_BLOCKING_HPP
#define PARAOS_QUEUE_BLOCKING_HPP

#include "paraos_queue.hpp"

namespace paraos {

template <typename T, typename ALLOCATOR = std::allocator<T>>
class QueueBlocking : public Queue<T, ALLOCATOR> {
 public:
  QueueBlocking(size_t max_elements_numb)
      : Queue<T, ALLOCATOR>(max_elements_numb),
        push_sem_{SemaphoreAttr{max_elements_numb}},
        pop_sem_{SemaphoreAttr{max_elements_numb}} {}

  virtual ~QueueBlocking() {}

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

  auto Push(T&& item, std::size_t timeout_ms) noexcept(noexcept(
      QueueBlocking<T, ALLOCATOR>::EmplaceBack(std::move(item)))) -> bool {
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
      noexcept(Queue<T, ALLOCATOR>::Push(item))) -> bool {
    bool is_pushed{false};

    if (Queue<T, ALLOCATOR>::IsFull()) {
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
      noexcept(Queue<T, ALLOCATOR>::Pop())) {
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

 private:
  Semaphore push_sem_;
  Semaphore pop_sem_;
};

}  // namespace paraos

#endif /* PARAOS_QUEUE_BLOCKING_HPP */
