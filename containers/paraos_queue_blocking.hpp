#ifndef PARAOS_QUEUE_BLOCKING_HPP
#define PARAOS_QUEUE_BLOCKING_HPP

#include "paraos_config.hpp"
#include "paraos_critical.hpp"
#include "paraos_mutex.hpp"
#include "paraos_queue.hpp"
#include "rtos_impl_mutex.hpp"

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
        pop_sem_{SemaphoreAttr{max_elements_numb}} {
    while (max_elements_numb > 0) {
      // необходимо отдать семафор pop_sem_ столько раз, сколько элементов может
      // хранить очередь. Иначе при вызове Push() семафор не будет получен
      // никогда.
      pop_sem_.Give();
      push_sem_.Take(0u);
      --max_elements_numb;
    }
  }

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
    assert(false && "Don't use EmplaceBack for blocking queue version");

    return false;
  }

  auto Push(T&& item, std::size_t timeout_ms) noexcept(
      noexcept(QueueBlocking<T, ALLOCATOR>::EmplaceBack(std::move(item))))
      -> bool override {
    paraosTRACE_MESSAGE("BlockingQueue full, POP semaphore waiting...");

    bool is_pushed{false};

    MutexGuard(mutex_push_, timeout_ms);
    if (pop_sem_.Take(timeout_ms)) {
      {
        const paraos::CriticalSection critical;
        paraosTRACE_MESSAGE("BlockingQueue POP semaphore taken, pushing...");
        is_pushed = Queue<T, ALLOCATOR>::Push(std::move(item));
      }

      push_sem_.Give();
    }

    return is_pushed;
  }

  auto Push(const T& item, std::size_t timeout_ms) noexcept(
      noexcept(Queue<T, ALLOCATOR>::Push(item))) -> bool override {
    bool is_pushed{false};

    // MutexGuard(mutex_push_, timeout_ms);
    if (pop_sem_.Take(timeout_ms)) {
      {
        const paraos::CriticalSection critical;
        paraosTRACE_MESSAGE("BlockingQueue POP semaphore taken, pushing...");
        is_pushed = Queue<T, ALLOCATOR>::Push(item);
      }

      push_sem_.Give();
    }

    return is_pushed;
  }

  auto Pop(std::size_t timeout_ms) noexcept(
      noexcept(Queue<T, ALLOCATOR>::Pop())) -> T override {
    paraosTRACE_MESSAGE("BlockingQueue taking PUSH semaphore");

    // MutexGuard(mutex_pop_, paraos::max_delay);
    if (push_sem_.Take(timeout_ms)) {
      decltype(Queue<T, ALLOCATOR>::Pop()) popped_value{};

      {
        const paraos::CriticalSection critical;
        paraosTRACE_MESSAGE("BlockingQueue PUSH semaphore taken successfully");

        if (!Queue<T, ALLOCATOR>::IsEmpty()) {
          popped_value = Queue<T, ALLOCATOR>::Pop();
        }
      }

      pop_sem_.Give();
      paraosTRACE_MESSAGE("BlockingQueue giving POP semaphore");
      return std::move(popped_value);
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
  MutexBase mutex_push_;
  MutexBase mutex_pop_;
};

}  // namespace paraos

#endif /* PARAOS_QUEUE_BLOCKING_HPP */
