#ifndef PARAOS_MESSAGE_BUFFER_HPP
#define PARAOS_MESSAGE_BUFFER_HPP

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

struct Message {
  Message(size_t size_in_bytes, paraos::IQueue<Message>* queue_ptr)
      : queue_ptr_{queue_ptr} {
    if (size_in_bytes > 0) {
      data_ptr_ = new (std::nothrow) std::uint8_t[size_in_bytes];

      if (data_ptr_) {
        size_in_bytes_ = size_in_bytes;
      }
    }

    paraosTRACE_MESSAGE("Message Ctor");
  }

  Message() { paraosTRACE_MESSAGE("Message (Empty Ctor)"); }

  virtual ~Message() {
    paraosTRACE_MESSAGE("~Message");

    delete[] data_ptr_;

    if (data_ptr_) {
      paraosTRACE_MESSAGE("~Message free");
    }
  };

  Message(const Message& other) noexcept {
    paraosTRACE_MESSAGE("Message Copy Ctor");

    data_ptr_ = new (std::nothrow) std::uint8_t[other.size_in_bytes_];
    if (data_ptr_) {
      memcpy(data_ptr_, other.data_ptr_, other.size_in_bytes_);
      size_in_bytes_ = other.size_in_bytes_;
    }
  }

  Message(Message&& other) noexcept {
    paraosTRACE_MESSAGE("Message Move Ctor");

    data_ptr_ = other.data_ptr_;
    size_in_bytes_ = other.size_in_bytes_;
    queue_ptr_ = other.queue_ptr_;

    other.data_ptr_ = nullptr;
  }

  Message& operator=(const Message& other) = delete;

  Message& operator=(Message&& other) noexcept {
    paraosTRACE_MESSAGE("Message Move operator");
    if (this == &other) {
      return *this;
    }

    delete data_ptr_;

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
    return static_cast<void*>(data_ptr_);
  }

  PARAOS_INLINE_TRIVIAL auto GetSize() const { return size_in_bytes_; }

 protected:
  paraos::IQueue<Message>* queue_ptr_{nullptr};

 private:
  std::uint8_t* data_ptr_{nullptr};
  size_t size_in_bytes_{0};
};

struct MessageWritable final : public Message {
  MessageWritable(size_t size_in_bytes, paraos::IQueue<Message>* queue_ptr)
      : Message{size_in_bytes, queue_ptr} {
    paraosTRACE_MESSAGE("MessageWritable Ctor");
  }

  ~MessageWritable() {
    paraosTRACE_MESSAGE("~MessageWritable Dtor");

    if (GetAddr() && !is_message_pop && queue_ptr_) {
      queue_ptr_->Push(std::move(*this));
    }
  }

  PARAOS_INLINE_TRIVIAL operator bool() const {
    if (GetAddr()) {
      return true;
    }
    return false;
  }

  void Pop() { is_message_pop = true; }

 private:
  bool is_message_pop{false};
};

struct QueueMessageBuffWrapper final : public IQueue<Message> {
 private:
  static_assert(std::has_virtual_destructor_v<IQueue<Message>>,
                "IQueue<T> must has virtual destruction");

 public:
  QueueMessageBuffWrapper(const size_t len) : queue_{len} {}
  ~QueueMessageBuffWrapper() = default;

  PARAOS_INLINE_TRIVIAL auto Push(Message&& elem) -> bool override {
    return queue_.Push(std::move(elem));
  }

  PARAOS_INLINE_TRIVIAL auto Push(const Message& elem) -> bool override {
    return queue_.Push(elem);
  }

  PARAOS_INLINE_TRIVIAL auto Pop() -> Message override {
    if (!IsEmpty()) {
      return queue_.Pop();
    }

    return Message{};
  }

  PARAOS_INLINE_TRIVIAL auto IsEmpty() -> bool override {
    return queue_.IsEmpty();
  }

  PARAOS_INLINE_TRIVIAL auto Size() -> size_t override { return queue_.Size(); }

  PARAOS_INLINE_TRIVIAL void Erase() override { queue_.Erase(); }

 private:
  paraos::Queue<Message> queue_;
};

template <typename QUEUE = paraos::QueueMessageBuffWrapper>
struct MessageBuffer {
  MessageBuffer(size_t buff_max_message_numb = 10)
      : queue_{buff_max_message_numb} {
    paraosTRACE_MESSAGE("MessageBuffer Ctor");
  }

  ~MessageBuffer() { paraosTRACE_MESSAGE("MessageBuffer Dtor"); }

  auto Alloc(size_t size_in_bytes) {
    paraosTRACE_MESSAGE("-- Alloc Message memory area");

    return MessageWritable{size_in_bytes, &queue_};
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
  QUEUE queue_;
};

}  // namespace paraos

#endif /* PARAOS_MESSAGE_BUFFER_HPP */
