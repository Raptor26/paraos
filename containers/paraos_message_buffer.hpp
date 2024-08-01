#ifndef PARAOS_MESSAGE_BUFFER_HPP
#define PARAOS_MESSAGE_BUFFER_HPP

#include <cstdint>
#include <cstring>
#include <queue>

#include "paraos_config.hpp"
#include "paraos_critical.hpp"
#include "paraos_mutex.hpp"
#include "paraos_queue.hpp"
#include "paraos_queue_blocking.hpp"
#include "paraos_trace.hpp"
#include "rtos_impl_mutex.hpp"

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

namespace paraos {

template <typename ALLOCATOR = std::allocator<std::uint8_t>>
struct Message {
  Message(
      std::size_t size_in_bytes,
      paraos::IQueueBlocking<Message<ALLOCATOR>> *queue_ptr)
      : queue_ptr_{queue_ptr} {
    if (size_in_bytes > 0) {
      data_ptr_ = alloc_traits::allocate(allocator_, size_in_bytes);

      if (data_ptr_) {
        size_in_bytes_ = size_in_bytes;
      }
    }

    paraosTRACE_MESSAGE("Message Ctor");
  }

  Message() { paraosTRACE_MESSAGE("Message (Empty Ctor)"); }

  virtual ~Message() {
    paraosTRACE_MESSAGE("~Message");

    if (data_ptr_) {
      paraosTRACE_MESSAGE("~Message free");
      alloc_traits::deallocate(allocator_, data_ptr_, size_in_bytes_);

      // debug only
      data_ptr_ = nullptr;
    }
  };

  Message(const Message<ALLOCATOR> &other) noexcept {
    paraosTRACE_MESSAGE("Message Copy Ctor");

    data_ptr_ = alloc_traits::allocate(allocator_, other.size_in_bytes_);

    if (data_ptr_) {
      memcpy(data_ptr_, other.data_ptr_, other.size_in_bytes_);
      size_in_bytes_ = other.size_in_bytes_;
    }
  }

  Message(Message<ALLOCATOR> &&other) noexcept {
    paraosTRACE_MESSAGE("Message Move Ctor");

    data_ptr_ = other.data_ptr_;
    size_in_bytes_ = other.size_in_bytes_;
    queue_ptr_ = other.queue_ptr_;

    other.data_ptr_ = nullptr;
  }

  Message &operator=(const Message<ALLOCATOR> &other) {
    if (this == &other) {
      return *this;
    }

    alloc_traits::deallocate(allocator_, data_ptr_, size_in_bytes_);

    data_ptr_ = alloc_traits::allocate(allocator_, other.size_in_bytes_);

    if (data_ptr_) {
      memcpy(data_ptr_, other.data_ptr_, other.size_in_bytes_);
      size_in_bytes_ = other.size_in_bytes_;
    }
  }

  Message &operator=(Message<ALLOCATOR> &&other) noexcept {
    paraosTRACE_MESSAGE("Message Move operator");
    if (this == &other) {
      return *this;
    }

    alloc_traits::deallocate(allocator_, data_ptr_, size_in_bytes_);

    data_ptr_ = other.data_ptr_;
    size_in_bytes_ = other.size_in_bytes_;
    queue_ptr_ = other.queue_ptr_;

    other.data_ptr_ = nullptr;

    return *this;
  }

  operator bool() const {
    if (data_ptr_) {
      return true;
    }
    return false;
  }

  PARAOS_INLINE_TRIVIAL auto GetAddr() const {
    return static_cast<void *>(data_ptr_);
  }

  PARAOS_INLINE_TRIVIAL auto GetSize() const { return size_in_bytes_; }

 protected:
  paraos::IQueueBlocking<Message<ALLOCATOR>> *queue_ptr_{nullptr};
  paraos::MutexBase *mutex_ptr_{nullptr};
  std::uint8_t *data_ptr_{nullptr};
  std::size_t size_in_bytes_{0};

 private:
  ALLOCATOR allocator_;
  using alloc_traits = std::allocator_traits<decltype(allocator_)>;
};

template <typename ALLOCATOR = std::allocator<std::uint8_t>>
struct MessageWritable final : public Message<ALLOCATOR> {
  MessageWritable(
      std::size_t size_in_bytes,
      paraos::IQueueBlocking<Message<ALLOCATOR>> *queue_ptr,
      paraos::MutexBase *mutex_ptr, paraos::SemaphoreBinary *sem_ptr,
      std::size_t timeout_to_push_ms)
      : Message<ALLOCATOR>{size_in_bytes, queue_ptr},
        timeout_to_push_ms_{timeout_to_push_ms},
        mutex_ptr_{mutex_ptr},
        sem_ptr_{sem_ptr} {
    paraosTRACE_MESSAGE("MessageWritable Ctor");
  }

  ~MessageWritable() {
    paraosTRACE_MESSAGE("~MessageWritable Dtor");

    Push();
  }

  MessageWritable(MessageWritable &&other) {
    paraosTRACE_MESSAGE("MessageWritable Move Ctor");

    this->data_ptr_ = other.data_ptr_;
    this->size_in_bytes_ = other.size_in_bytes_;
    this->queue_ptr_ = other.queue_ptr_;
    mutex_ptr_ = other.mutex_ptr_;
    sem_ptr_ = other.sem_ptr_;

    other.data_ptr_ = nullptr;
    is_message_pop = false;
  }

  PARAOS_INLINE_TRIVIAL
  operator bool() const {
    if (this->GetAddr()) {
      return true;
    }
    return false;
  }

  auto Push(std::size_t timeout_ms) -> bool {
    bool is_message_pushed_in_buff{false};
    if (this->GetAddr() && !is_message_pop && this->queue_ptr_ && mutex_ptr_ &&
        sem_ptr_) {
      const paraos::CriticalSection critical;  // todo delete me
      paraosTRACE_MESSAGE("MessageWritable Push in buffer");
      is_message_pushed_in_buff =
          this->queue_ptr_->Push(std::move(*this), timeout_ms);

      //   sem_ptr_->Give();
    }

    if (mutex_ptr_) {
      mutex_ptr_->Unlock();
    }

    return is_message_pushed_in_buff;
  }

  auto Push() -> bool { return Push(timeout_to_push_ms_); }

  void Pop() { is_message_pop = true; }

 private:
  bool is_message_pop{false};
  std::size_t timeout_to_push_ms_{0};
  paraos::MutexBase *mutex_ptr_{nullptr};
  paraos::SemaphoreBinary *sem_ptr_{nullptr};
};

template <
    typename MESSAGE_ALLOCATOR = std::allocator<std::uint8_t>,
    typename QUEUE_ALLOCATOR = std::allocator<Message<MESSAGE_ALLOCATOR>>>
struct QueueMessageBuffWrapper final
    : public IQueueBlocking<Message<MESSAGE_ALLOCATOR>> {
 public:
  QueueMessageBuffWrapper(const std::size_t len) : queue_{len} {}
  ~QueueMessageBuffWrapper() = default;

  operator bool() const { return static_cast<bool>(queue_); }

  PARAOS_INLINE_TRIVIAL auto Push(
      Message<MESSAGE_ALLOCATOR> &&elem,
      std::size_t timeout_ms) -> bool override {
    return queue_.Push(std::move(elem), timeout_ms);
  }

  PARAOS_INLINE_TRIVIAL auto Push(
      const Message<MESSAGE_ALLOCATOR> &elem,
      std::size_t timeout_ms) -> bool override {
    return queue_.Push(elem, timeout_ms);
  }

  PARAOS_INLINE_TRIVIAL auto Pop(std::size_t timeout_ms)
      -> Message<MESSAGE_ALLOCATOR> override {
    return queue_.Pop(timeout_ms);
  }

  PARAOS_INLINE_TRIVIAL auto IsEmpty() -> bool override {
    return queue_.IsEmpty();
  }

  PARAOS_INLINE_TRIVIAL auto IsFull() -> bool override {
    return queue_.IsFull();
  }

  PARAOS_INLINE_TRIVIAL auto Size() -> std::size_t override {
    return queue_.Size();
  }

  PARAOS_INLINE_TRIVIAL void Erase() override { queue_.Erase(); }

 private:
  paraos::QueueBlocking<Message<MESSAGE_ALLOCATOR>, QUEUE_ALLOCATOR> queue_;
};

template <
    typename BUFFER_ALLOCATOR = std::allocator<std::uint8_t>,
    typename QUEUE_ALLOCATOR = std::allocator<Message<BUFFER_ALLOCATOR>>>
struct MessageBuffer {
  MessageBuffer(std::size_t buff_max_message_numb = 10)
      : queue_{buff_max_message_numb} {
    paraosTRACE_MESSAGE("MessageBuffer Ctor");

    // while (buff_max_message_numb > 0u) {
    // sem_.Give();
    //   --buff_max_message_numb;
    // }
  }

  ~MessageBuffer() { paraosTRACE_MESSAGE("MessageBuffer Dtor"); }

  operator bool() const { return static_cast<bool>(queue_); }

  auto Alloc(std::size_t size_in_bytes, std::size_t timeout_ms) {
    paraosTRACE_MESSAGE("-- Alloc Message memory area");

    if (mutex_.Lock(timeout_ms)) {
      // Берем мьютекс до тех пор, пока сообщение не будет записано в очередь
      bool is_sem_taken{true};
      if (IsFull()) {
        is_sem_taken = sem_.Take(timeout_ms);
      }
      if (is_sem_taken) {
        // assert(!IsFull() && "Buffer can't be full, we successfully take
        // sem");
        auto message = MessageWritable<BUFFER_ALLOCATOR>{
            size_in_bytes, &queue_, &mutex_, &sem_, timeout_ms};

        if (!message) {
          return MessageWritable<BUFFER_ALLOCATOR>{
              0u, nullptr, &mutex_, nullptr, 0u};
        }
        return std::move(message);
      }
    }
    return MessageWritable<BUFFER_ALLOCATOR>{0u, nullptr, &mutex_, nullptr, 0u};
  }

  auto Pop(std::size_t timeout_ms) {
    paraosTRACE_MESSAGE("-- Pop Message from buffer");

    // Лямбда-функция ниже будет вызвана сразу после оператора return
    Finally pop_from_queue{[&] {
      // Кода извлекается элемент из очереди, это означает что в буфере
      // появиться свободное место. Тогда необходимо 'отдать'
      // соответствующий семафор.
      sem_.Give();
    }};

    return queue_.Pop(timeout_ms);
  }

  auto IsEmpty() { return queue_.IsEmpty(); }
  auto IsFull() { return queue_.IsFull(); }

  auto Size() { return queue_.Size(); }

  void Erase() { queue_.Erase(); }

 private:
  paraos::QueueBlocking<Message<BUFFER_ALLOCATOR>, QUEUE_ALLOCATOR> queue_;
  paraos::MutexBase mutex_;
  paraos::MutexBase mutex_pop_;

  /// @brief Семафор можно взять только в том случае, если в очереди есть хотя
  /// бы одна свободная ячейка.
  paraos::SemaphoreBinary sem_;
};

}  // namespace paraos

#endif /* PARAOS_MESSAGE_BUFFER_HPP */
