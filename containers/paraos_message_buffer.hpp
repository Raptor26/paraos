/// @file paraos_message_buffer.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2024 Stilsoft
///
/// MIT License:
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the 'Software'), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.

#ifndef PARAOS_MESSAGE_BUFFER_HPP
#define PARAOS_MESSAGE_BUFFER_HPP

#include <cinttypes>
#include <vector>

#include "paraos_config.hpp"
#include "paraos_mutex.hpp"
#include "paraos_mutex_raii.hpp"
#include "paraos_queue_blocking_v3.hpp"
#include "paraos_thread.hpp"

namespace paraos {

/// @brief Message container. Manage memory, requested from ALLOCATOR.
/// @tparam ALLOCATOR - Allocator for request memory.
template <typename ALLOCATOR = std::allocator<std::uint8_t>>
class Message {
  ALLOCATOR allocator_;
  using alloc_traits = std::allocator_traits<decltype(allocator_)>;

 public:
  /// @brief Запрашивает из кучи размер памяти, указанный в size_in_bytes
  /// @param[in] size_in_bytes: Размер области памяти в байтах, который
  /// необходимо выделить из аллокатора памяти.
  Message(const std::size_t size_in_bytes)
      : data_ptr_{nullptr}, size_in_bytes_{size_in_bytes} {
    SafeAllocate();
  }

  Message() : data_ptr_{nullptr}, size_in_bytes_{0} {}

  virtual ~Message() { SafeDeallocate(); }

  Message(const Message &other)
      : data_ptr_{nullptr}, size_in_bytes_{other.size_in_bytes_} {
    SafeAllocate();

    if (data_ptr_) {
      // After memory allocated, need copy bytes in allocated memory area from
      // other memory area.
      memcpy(data_ptr_, other.data_ptr_, size_in_bytes_);
    }
  }

  Message(Message &&other)
      : data_ptr_{other.data_ptr_}, size_in_bytes_{other.size_in_bytes_} {
    other.data_ptr_ = nullptr;
  }

  Message &operator=(const Message &other) = delete;
  Message &operator=(Message &&other) = delete;

  operator bool() const {
    bool is_ready{false};

    if (data_ptr_) {
      is_ready = true;
    }

    return is_ready;
  }

  /// @brief Возвращает адрес выделенной области памяти.
  /// @return Указатель типа void.
  PARAOS_INLINE_TRIVIAL void *Data() const {
    return static_cast<void *>(data_ptr_);
  }

  /// @brief Возвращает размер выделенной области памяти в байтах.
  /// @return Количество байт, выделенные по адресу, который возвращает метод
  /// Addr().
  PARAOS_INLINE_TRIVIAL size_t Size() const { return size_in_bytes_; }

  /// @brief Принудительно освобождает область памяти, выделенную под сообщение.
  /// После вызова данного метода, объект становиться не валидным.
  PARAOS_INLINE_TRIVIAL void Free() { SafeDeallocate(); }

 private:
  PARAOS_INLINE_TRIVIAL void SafeAllocate() {
    if (size_in_bytes_ > 0u) {
      data_ptr_ = alloc_traits::allocate(allocator_, size_in_bytes_);
    }
  }

  PARAOS_INLINE_TRIVIAL void SafeDeallocate() {
    if (data_ptr_) {
      alloc_traits::deallocate(allocator_, data_ptr_, size_in_bytes_);
      data_ptr_ = nullptr;
    }
  }

  /// @brief Указатель на выделенную область памяти под хранение сообщения.
  std::uint8_t *data_ptr_;

  /// @brief Размер выделенной области памяти в байтах.
  const std::size_t size_in_bytes_;
};

/// @brief Message object, returned by MessageBuffer when user code calls
/// Alloc().
/// @tparam ALLOCATOR
template <typename ALLOCATOR = std::allocator<std::uint8_t>>
class MessageWritable final {
 public:
  MessageWritable(
      const std::size_t size_in_bytes,
      paraos::v3::IQueueBlocking<Message<ALLOCATOR>> &queue)
      : message_{size_in_bytes}, queue_{queue} {}

  ~MessageWritable() { TryPush(); }

  MessageWritable(const MessageWritable &other) = delete;
  MessageWritable(MessageWritable &&other) = delete;
  MessageWritable &operator=(const MessageWritable &other) = delete;
  MessageWritable &operator=(MessageWritable &&other) = delete;

  operator bool() const { return message_; }

  PARAOS_INLINE_TRIVIAL void *Data() { return message_.Data(); }
  PARAOS_INLINE_TRIVIAL size_t Size() { return message_.Size(); }

  /// @brief Try push message in buffer. Message will push if queue has space.
  ///
  /// @note  User code not necessary call this method. TryPush() automaticaly
  /// calls in dtor.
  ///
  /// @details If user successfully alloc space for message, this does not mean
  /// that this message will be successfully move in buffer. If between
  /// IMessageBuffer.Alloc() and TryPush() any thread full queue, TryPush()
  /// can't push this message in buffer and return false. In any case, resources
  /// will automaticaly free.
  ///
  /// @note If need alloc and push message atomy, user code need call
  /// IMessageBuffer.Alloc() and TryPush() inside one critical section.
  ///
  /// @param[in] is_isr: Set true if calls from isr.
  ///
  /// @return Return true if message successfully pushed in buffer, false in
  /// otherwise.
  PARAOS_INLINE_OPERATIONS bool TryPush(bool is_isr = false) {
    bool is_message_pushed{false};

    // If user calls TryPush(), that's mean when calls dtor, TryPush() will
    // calls again. For this reason need check message_ validation.
    if (message_) {
      is_message_pushed = queue_.TryPush(std::move(message_), is_isr);
      // Nothin to push again, free resources.
      Free();
    }

    return is_message_pushed;
  }

  /// @brief Пользователь может вызвать данный метод если передумал отправлять
  /// сообщение в буфер.
  PARAOS_INLINE_TRIVIAL void Free() { message_.Free(); }

 private:
  Message<ALLOCATOR> message_;
  paraos::v3::IQueueBlocking<Message<ALLOCATOR>> &queue_;
};

/// @brief Message buffer base class. Contained API for buffer.
///
/// @tparam BUFFER_ALLOCATOR: Memory allocator for request memory for each
/// message.
template <typename BUFFER_ALLOCATOR = std::allocator<std::uint8_t>>
class IMessageBuffer {
 public:
  virtual ~IMessageBuffer() = default;

  IMessageBuffer(const IMessageBuffer &other) = delete;
  IMessageBuffer(IMessageBuffer &&other) = delete;
  IMessageBuffer &operator=(const IMessageBuffer &other) = delete;
  IMessageBuffer &operator=(IMessageBuffer &&other) = delete;

  /// @brief  Request memory from allocator, witch set in MessageBuffer ctor.
  ///
  /// @param[in] size_in_bytes: Requested memory size in bytes.
  ///
  /// @return Return container. Note - container way not contained requested
  /// memory. Befor start any operations with MessageWritable object, check his
  /// validation (use operator bool).
  PARAOS_INLINE_TRIVIAL auto Alloc(std::size_t size_in_bytes) {
    return MessageWritable(size_in_bytes, queue_);
  }

  /// @brief Return message container if any data available in buffer.
  ///
  /// @param[in] timeout_ms: Timeout for wait any data in buffer if no data
  /// available in calls time.
  ///
  /// @return Return std::optional object. If no data was read, std::optional
  /// will empty. In otherwise std::optional contained message.
  PARAOS_INLINE_TRIVIAL auto Pop(std::size_t timeout_ms) {
    return queue_.Pop(timeout_ms);
  }

  PARAOS_INLINE_TRIVIAL bool IsFull() { return queue_.IsFull(); }
  PARAOS_INLINE_TRIVIAL bool IsEmpty() { return queue_.IsEmpty(); }

 protected:
  IMessageBuffer(paraos::v3::IQueueBlocking<Message<BUFFER_ALLOCATOR>> &queue)
      : queue_{queue} {}

 private:
  paraos::v3::IQueueBlocking<Message<BUFFER_ALLOCATOR>> &queue_;
};

/// @brief Message buffer class.
///
/// @tparam QUEUE_SIZE: Max message numb for contained in buffer in same time.
/// @tparam BUFFER_ALLOCATOR: Memory allocator for request memory for each
/// message.
template <
    const std::size_t QUEUE_SIZE,
    typename BUFFER_ALLOCATOR = std::allocator<std::uint8_t>>
class MessageBuffer final : public IMessageBuffer<BUFFER_ALLOCATOR> {
  static_assert(
      QUEUE_SIZE > 0, "Message contained counter must be greater then '0'");

 public:
  MessageBuffer() : IMessageBuffer<BUFFER_ALLOCATOR>{queue_} {}

  ~MessageBuffer() = default;

  MessageBuffer(const MessageBuffer &other) = delete;
  MessageBuffer(MessageBuffer &&other) = delete;
  MessageBuffer &operator=(const MessageBuffer &other) = delete;
  MessageBuffer &operator=(MessageBuffer &&other) = delete;

  operator bool() const { return queue_; }

 private:
  paraos::v3::QueueBlocking<Message<BUFFER_ALLOCATOR>, QUEUE_SIZE> queue_;
};

}  // namespace paraos

#endif /* PARAOS_MESSAGE_BUFFER_HPP */
