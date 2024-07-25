#ifndef PARAOS_MESSAGE_BUFFER_HPP
#define PARAOS_MESSAGE_BUFFER_HPP

#include <cstdint>
#include <cstring>
#include <queue>


#include "paraos_config.hpp"
#include "paraos_critical.hpp"
#include "paraos_queue.hpp"
#include "paraos_trace.hpp"

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

#if defined(_WIN32) || defined(_WIN64)
#include "win/paraos_critical.hpp"
#endif

namespace paraos {

template <typename ALLOCATOR = std::allocator<std::uint8_t>> struct Message {
  Message(size_t size_in_bytes, paraos::IQueue<Message<ALLOCATOR>> *queue_ptr)
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

    alloc_traits::deallocate(allocator_, data_ptr_, size_in_bytes_);

    if (data_ptr_) {
      paraosTRACE_MESSAGE("~Message free");
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

  Message &operator=(const Message<ALLOCATOR> &other) = delete;

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
  paraos::IQueue<Message<ALLOCATOR>> *queue_ptr_{nullptr};

private:
  ALLOCATOR allocator_;
  using alloc_traits = std::allocator_traits<decltype(allocator_)>;

  std::uint8_t *data_ptr_{nullptr};
  size_t size_in_bytes_{0};
};

template <typename ALLOCATOR = std::allocator<std::uint8_t>>
struct MessageWritable final : public Message<ALLOCATOR> {
  MessageWritable(size_t size_in_bytes,
                  paraos::IQueue<Message<ALLOCATOR>> *queue_ptr)
      : Message<ALLOCATOR>{size_in_bytes, queue_ptr} {
    paraosTRACE_MESSAGE("MessageWritable Ctor");
  }

  ~MessageWritable() {
    paraosTRACE_MESSAGE("~MessageWritable Dtor");

    if (this->GetAddr() && !is_message_pop && this->queue_ptr_) {
      this->queue_ptr_->Push(std::move(*this));
    }
  }

  PARAOS_INLINE_TRIVIAL operator bool() const {
    if (this->GetAddr()) {
      return true;
    }
    return false;
  }

  void Pop() { is_message_pop = true; }

private:
  bool is_message_pop{false};
};

template <typename MESSAGE_ALLOCATOR = std::allocator<std::uint8_t>,
          typename QUEUE_ALLOCATOR = std::allocator<Message<MESSAGE_ALLOCATOR>>>
struct QueueMessageBuffWrapper final
    : public IQueue<Message<MESSAGE_ALLOCATOR>> {
private:
  static_assert(
      std::has_virtual_destructor_v<IQueue<Message<MESSAGE_ALLOCATOR>>>,
      "IQueue<T> must have virtual destruction");

public:
  QueueMessageBuffWrapper(const size_t len) : queue_{len} {}
  ~QueueMessageBuffWrapper() = default;

  PARAOS_INLINE_TRIVIAL auto
  Push(Message<MESSAGE_ALLOCATOR> &&elem) -> bool override {
    return queue_.Push(std::move(elem));
  }

  PARAOS_INLINE_TRIVIAL auto
  Push(const Message<MESSAGE_ALLOCATOR> &elem) -> bool override {
    return queue_.Push(elem);
  }

  PARAOS_INLINE_TRIVIAL auto Pop() -> Message<MESSAGE_ALLOCATOR> override {
    if (!IsEmpty()) {
      return queue_.Pop();
    }

    return Message<MESSAGE_ALLOCATOR>{};
  }

  PARAOS_INLINE_TRIVIAL auto IsEmpty() -> bool override {
    return queue_.IsEmpty();
  }

  PARAOS_INLINE_TRIVIAL auto Size() -> size_t override { return queue_.Size(); }

  PARAOS_INLINE_TRIVIAL void Erase() override { queue_.Erase(); }

private:
  paraos::Queue<Message<MESSAGE_ALLOCATOR>, QUEUE_ALLOCATOR> queue_;
};

template <typename BUFFER_ALLOCATOR = std::allocator<std::uint8_t>,
          typename QUEUE_ALLOCATOR = std::allocator<Message<BUFFER_ALLOCATOR>>>
struct MessageBuffer {
  MessageBuffer(size_t buff_max_message_numb = 10)
      : queue_{buff_max_message_numb} {
    paraosTRACE_MESSAGE("MessageBuffer Ctor");
  }

  ~MessageBuffer() { paraosTRACE_MESSAGE("MessageBuffer Dtor"); }

  auto Alloc(size_t size_in_bytes) {
    paraosTRACE_MESSAGE("-- Alloc Message memory area");

    return MessageWritable<BUFFER_ALLOCATOR>{size_in_bytes, &queue_};
  }

  auto Pop() {
    paraosTRACE_MESSAGE("-- Pop Message from buffer");

    const CriticalSection critical;
    return queue_.Pop();
  }

  auto IsEmpty() { return queue_.IsEmpty(); }

  auto Size() { return queue_.Size(); }

  void Erase() { queue_.Erase(); }

private:
  paraos::QueueMessageBuffWrapper<BUFFER_ALLOCATOR, QUEUE_ALLOCATOR> queue_;
};

} // namespace paraos

#endif /* PARAOS_MESSAGE_BUFFER_HPP */
