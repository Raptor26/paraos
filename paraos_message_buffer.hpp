#ifndef PARAOS_MESSAGE_BUFFER_HPP
#define PARAOS_MESSAGE_BUFFER_HPP

#include <queue>

#include "critical.hpp"

#ifdef paraosTRACE_ENABLE
#include <iostream>
#endif

#if defined(_WIN32) && defined(_WIN64)
#include "win/critical.hpp"
#endif

namespace paraos {

/// @brief Finally with no overhead for heap memory
/// @tparam ActTy
template <typename ActTy>
struct Finally {
  ActTy act_;
  explicit Finally(ActTy act) : act_{std::move(act)} {}
  ~Finally() { act_(); }
};

struct MessageBuffer;

template <typename T>
struct IQueue {
  virtual bool Push(const T& element) { return false; }
};

struct Message {
  Message(size_t size_in_bytes, std::queue<Message>* mother_buff_ptr)
      : queue_buff_ptr_{mother_buff_ptr} {
    if (size_in_bytes > 0) {
      data_ptr_ = malloc(size_in_bytes);

      if (data_ptr_) {
        size_in_bytes_ = size_in_bytes;
      }
    }
#ifdef paraosTRACE_ENABLE
    std::cout << "Message" << std::endl;
#endif
  }

  Message() {
#ifdef paraosTRACE_ENABLE
    std::cout << "Message (Empty Ctor)" << std::endl;
#endif
  }

  virtual ~Message() {
#ifdef paraosTRACE_ENABLE
    std::cout << "~Message" << std::endl;
#endif

    if (data_ptr_) {
#ifdef paraosTRACE_ENABLE
      std::cout << "~Message free" << std::endl;
#endif
      free(data_ptr_);
    }
  };

  Message(const Message& other) noexcept {
#ifdef paraosTRACE_ENABLE
    std::cout << "Message Copy Ctor" << std::endl;
#endif

    data_ptr_ = malloc(other.size_in_bytes_);
    if (data_ptr_) {
      memcpy(data_ptr_, other.data_ptr_, other.size_in_bytes_);
      size_in_bytes_ = other.size_in_bytes_;
    }
  }

  Message(Message&& other) noexcept {
#ifdef paraosTRACE_ENABLE
    std::cout << "Message Move Ctor" << std::endl;
#endif

    data_ptr_ = other.data_ptr_;
    size_in_bytes_ = other.size_in_bytes_;
    queue_buff_ptr_ = other.queue_buff_ptr_;

    other.data_ptr_ = nullptr;
  }

  Message& operator=(const Message& other) = delete;

  Message& operator=(Message&& other) noexcept {
#ifdef paraosTRACE_ENABLE
    std::cout << "Message Move operator" << std::endl;
#endif
    if (this == &other) {
      return *this;
    }

    if (data_ptr_) {
      free(data_ptr_);
    }

    data_ptr_ = other.data_ptr_;
    size_in_bytes_ = other.size_in_bytes_;
    queue_buff_ptr_ = other.queue_buff_ptr_;

    other.data_ptr_ = nullptr;

    return *this;
  }

  operator bool() const {
    if (data_ptr_) {
      return true;
    }
    return false;
  }

  auto GetAddr() const { return data_ptr_; }
  auto GetSize() const { return size_in_bytes_; }

 private:
  void* data_ptr_{nullptr};
  size_t size_in_bytes_{0};

 protected:
  std::queue<Message>* queue_buff_ptr_{nullptr};
};

struct MessageWritable final : public Message {
  MessageWritable(size_t size_in_bytes, std::queue<Message>* mother_buff_ptr)
      : Message{size_in_bytes, mother_buff_ptr} {
#ifdef paraosTRACE_ENABLE
    std::cout << "MessageWritable" << std::endl;
#endif
  }

  ~MessageWritable() {
#ifdef paraosTRACE_ENABLE
    std::cout << "~MessageWritable" << std::endl;
#endif

    if (GetAddr() && !is_message_pop && queue_buff_ptr_) {
      queue_buff_ptr_->push(std::move(*this));
    }
  }

  operator bool() const {
    if (GetAddr()) {
      return true;
    }
    return false;
  }

  void Pop() { is_message_pop = true; }

 private:
  bool is_message_pop{false};
};

struct MessageBuffer {
  MessageBuffer(size_t buff_max_message_numb = 10) {
#ifdef paraosTRACE_ENABLE
    std::cout << "MessageBuffer Ctor" << std::endl;
#endif
  }

  ~MessageBuffer() {
#ifdef paraosTRACE_ENABLE
    std::cout << "MessageBuffer Dtor" << std::endl;
#endif
  }

  std::queue<Message> queue_;

  auto Alloc(size_t size_in_bytes) -> MessageWritable {
#ifdef paraosTRACE_ENABLE
    std::cout << "-- Alloc Message memory area" << std::endl;
#endif

    return MessageWritable{size_in_bytes, &queue_};
  }

  auto Pop() -> Message {
#ifdef paraosTRACE_ENABLE
    std::cout << "-- Pop Message from buffer" << std::endl;
#endif

    const CriticalSection critical;
    if (!queue_.empty()) {
      auto& read = queue_.front();

      // Нам необходимо вызвать queue_.pop(); после оператора return. Для
      // решения поставленной задачи воспользуемся классом Finally и
      // лямбда-выражением
      Finally pop_from_queue{[&] {
#ifdef paraosTRACE_ENABLE
        std::cout << "Call pop() for queue" << std::endl;
#endif
        // Данный метод будет вызван в деструкторе переменной 'pop_from_queue'
        queue_.pop();
      }};

      return std::move(read);

      // Dtor 'pop_from_queue' call queue_.pop();
    }

    return Message{};
  }

  auto IsBufferEmpty() { return queue_.empty(); }

  auto Size() { return queue_.size(); }

  void Erase() {
    while (!queue_.empty()) {
      queue_.pop();
    }
  }
};

}  // namespace paraos

#endif /* PARAOS_MESSAGE_BUFFER_HPP */
