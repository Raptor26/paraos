/// @file paraos_message_buffer.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_MESSAGE_BUFFER_HPP
#define PARAOS_MESSAGE_BUFFER_HPP

#include <cinttypes>
#include <type_traits>
#include <utility>
#include <vector>

#include "paraos_config.hpp"
#include "paraos_isr.hpp"
#include "paraos_queue_blocking.hpp"

namespace paraos {

/// @brief Message container. Manage memory, requested from ALLOCATOR.
/// @tparam ALLOCATOR - Allocator for request memory.
template <typename ALLOCATOR = std::allocator<std::uint8_t>>
class message {
  ALLOCATOR allocator_;
  using alloc_traits = std::allocator_traits<ALLOCATOR>;

  static_assert(
      sizeof(typename std::allocator_traits<ALLOCATOR>::value_type) ==
          sizeof(std::uint8_t),
      "ALLOCATOR must allocate memory per byte");

 public:
  using value_type = std::uint8_t;
  using pointer = std::uint8_t*;
  using reference = std::uint8_t&;
  using iterator = pointer;
  using const_iterator = const pointer;
  using iterator_category = std::random_access_iterator_tag;

  // ---------------------------------------------------------------------------

  /// @brief Запрашивает из кучи размер памяти, указанный в size_in_bytes
  /// @param[in] size_in_bytes: Размер области памяти в байтах, который
  /// необходимо выделить из аллокатора памяти.
  explicit message(const std::size_t size_in_bytes)
      : size_in_bytes_{size_in_bytes},
        data_ptr_{safe_allocate(size_in_bytes_)} {}

  // ---------------------------------------------------------------------------

  message() : data_ptr_{nullptr}, size_in_bytes_{0} {}

  // ---------------------------------------------------------------------------

  virtual ~message() { safe_deallocate(); }

  // ---------------------------------------------------------------------------

  message(const message& other) noexcept
      : size_in_bytes_{other.size_in_bytes_},
        data_ptr_{safe_allocate(size_in_bytes_)} {
    if (data_ptr_ != nullptr) {
      // After memory allocated, need copy bytes in allocated memory area from
      // other memory area.
      memcpy(data_ptr_, other.data_ptr_, size_in_bytes_);
    }
  }

  message(message&& other) noexcept
      : size_in_bytes_{other.size_in_bytes_}, data_ptr_{other.data_ptr_} {
    other.data_ptr_ = nullptr;
  }

  auto operator=(const message& other) -> message& = delete;
  auto operator=(message&& other) noexcept -> message& {
    if (&other != this) {
      this->safe_deallocate();

      this->data_ptr_ = other.data_ptr_;
      other.data_ptr_ = nullptr;

      this->size_in_bytes_ = other.size_in_bytes_;
    }

    return *this;
  }

  // ---------------------------------------------------------------------------
  [[nodiscard]] auto begin() noexcept {
    return reinterpret_cast<iterator>(data_ptr_);
  }

  [[nodiscard]] auto begin() const noexcept {
    return reinterpret_cast<const_iterator>(data_ptr_);
  }

  [[nodiscard]] auto cbegin() const noexcept {
    return reinterpret_cast<const_iterator>(data_ptr_);
  }

  [[nodiscard]] auto end() noexcept {
    return reinterpret_cast<iterator>(begin() + size_in_bytes_);
  }

  [[nodiscard]] auto end() const noexcept {
    return reinterpret_cast<const_iterator>(begin() + size_in_bytes_);
  }

  [[nodiscard]] auto cend() const noexcept {
    return reinterpret_cast<const_iterator>(begin() + size_in_bytes_);
  }

  // ---------------------------------------------------------------------------

  explicit operator bool() const noexcept {
    bool is_ready{false};

    if (data_ptr_ != nullptr) {
      is_ready = true;
    }

    return is_ready;
  }

  // ---------------------------------------------------------------------------

  /// @brief Возвращает адрес выделенной области памяти.
  /// @return Указатель типа void.
  template <typename USER_DATA_TYPE = std::uint8_t>
  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto data() const noexcept {
    return reinterpret_cast<USER_DATA_TYPE*>(data_ptr_);
  }

  /// @brief Возвращает размер выделенной области памяти в байтах.
  /// @return Количество байт, выделенные по адресу, который возвращает метод
  /// Addr().
  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto size() const noexcept {
    return size_in_bytes_;
  }

  /// @brief Принудительно освобождает область памяти, выделенную под сообщение.
  /// После вызова данного метода, объект становиться не валидным.
  PARAOS_INLINE_TRIVIAL void free() noexcept { safe_deallocate(); }

  // Backward-compatible deprecated forwarding methods.
  template <typename USER_DATA_TYPE = std::uint8_t>
  [[nodiscard]] PARAOS_DEPRECATED("use data()") PARAOS_INLINE_TRIVIAL
      auto Data() const noexcept {
    return data<USER_DATA_TYPE>();
  }

  [[nodiscard]] PARAOS_DEPRECATED("use size()") PARAOS_INLINE_TRIVIAL
      auto Size() const noexcept {
    return size();
  }

  PARAOS_DEPRECATED("use free()") PARAOS_INLINE_TRIVIAL void Free() noexcept {
    free();
  }

  // ---------------------------------------------------------------------------

 private:
  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto safe_allocate(
      std::size_t size_in_bytes) noexcept -> pointer {
    if (size_in_bytes > 0U) {
      return alloc_traits::allocate(allocator_, size_in_bytes);
    }

    return nullptr;
  }

  // ---------------------------------------------------------------------------

  PARAOS_INLINE_TRIVIAL void safe_deallocate() {
    if (data_ptr_ != nullptr) {
      alloc_traits::deallocate(allocator_, data_ptr_, size_in_bytes_);
      data_ptr_ = nullptr;
    }
  }

  /// @brief Размер выделенной области памяти в байтах.
  std::size_t size_in_bytes_;

  /// @brief Указатель на выделенную область памяти под хранение сообщения.
  pointer data_ptr_;
};

template <typename ALLOCATOR = std::allocator<std::uint8_t>>
using Message PARAOS_DEPRECATED("use paraos::message") = message<ALLOCATOR>;

/// @brief Message object, returned by message_buffer when user code calls
/// alloc().
/// @tparam ALLOCATOR
/// @tparam QUEUE_SIZE
template <
    typename ALLOCATOR = std::allocator<std::uint8_t>,
    const std::size_t QUEUE_SIZE = 1U>
class message_writable final {
  using message_type = message<ALLOCATOR>;

  static_assert(
      sizeof(typename std::allocator_traits<ALLOCATOR>::value_type) ==
          sizeof(std::uint8_t),
      "ALLOCATOR must allocate memory per byte");

 public:
  using value_type = typename message_type::value_type;
  using pointer = typename message_type::pointer;
  using reference = typename message_type::reference;
  using iterator = typename message_type::iterator;
  using const_iterator = typename message_type::const_iterator;
  using iterator_category = typename message_type::iterator_category;

  message_writable(
      const std::size_t size_in_bytes,
      paraos::queue_blocking_base<message<ALLOCATOR>, QUEUE_SIZE>& queue)
      : message_{size_in_bytes}, queue_{queue} {}

  ~message_writable() { try_push(); }

  message_writable(const message_writable& other) = delete;

  message_writable(message_writable&& other) noexcept
      : message_{std::move(other.message_)}, queue_{other.queue_} {}

  auto operator=(const message_writable& other) -> message_writable& = delete;
  auto operator=(message_writable&& other) -> message_writable& = delete;

  explicit operator bool() const { return static_cast<bool>(message_); }

  [[nodiscard]] auto begin() { return message_.begin(); }
  [[nodiscard]] auto begin() const { return message_.begin(); }
  [[nodiscard]] auto cbegin() const { return message_.cbegin(); }
  [[nodiscard]] auto end() { return message_.end(); }
  [[nodiscard]] auto end() const { return message_.end(); }
  [[nodiscard]] auto cend() const { return message_.cend(); }

  template <typename USER_DATA_TYPE = std::uint8_t>
  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto data() const {
    return message_.template data<USER_DATA_TYPE>();
  }

  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto size() const {
    return message_.size();
  }

  /// @brief Try push message in buffer. Message will push if queue has space.
  ///
  /// @note  User code not necessary call this method. try_push() automatically
  /// calls in dtor.
  ///
  /// @details If user successfully alloc space for message, this does not mean
  /// that this message will be successfully move in buffer. If between
  /// message_buffer_base.alloc() and try_push() any thread full queue,
  /// try_push() can't push this message in buffer and return false. In any
  /// case, resources will automatically free.
  ///
  /// @note If need alloc and push message atomy, user code need call
  /// message_buffer_base.alloc() and try_push() inside one critical section.
  ///
  /// @param[in] is_isr: Set true if calls from isr.
  ///
  /// @return Return true if message successfully pushed in buffer, false in
  /// otherwise.
  PARAOS_INLINE_OPERATIONS auto try_push(bool is_isr = false) noexcept {
    paraos::isr_bool is_message_pushed{false};

    // If user calls try_push(), that's mean when calls dtor, try_push() will
    // calls again. For this reason need check message_ validation.
    if (message_) {
      is_message_pushed = queue_.try_push(std::move(message_), is_isr);

      // Nothin to push again, resources will be free automatically in message_
      // dtor if needing.
    }

    // If message didn't push, dtor of the message_ free resources
    // automatically,

    return is_message_pushed;
  }

  /// @brief Пользователь может вызвать данный метод если передумал отправлять
  /// сообщение в буфер.
  PARAOS_INLINE_TRIVIAL void free() noexcept(
      std::is_nothrow_invocable_v<
          decltype(&message_type::free), message_type>) {
    message_.free();
  }

  // Backward-compatible deprecated forwarding methods.
  template <typename USER_DATA_TYPE = std::uint8_t>
  [[nodiscard]] PARAOS_DEPRECATED("use data()") PARAOS_INLINE_TRIVIAL
      auto Data() const {
    return data<USER_DATA_TYPE>();
  }

  [[nodiscard]] PARAOS_DEPRECATED("use size()") PARAOS_INLINE_TRIVIAL
      auto Size() const {
    return size();
  }

  PARAOS_DEPRECATED("use try_push()")
  PARAOS_INLINE_OPERATIONS auto TryPush(bool is_isr = false) noexcept {
    return try_push(is_isr);
  }

  PARAOS_DEPRECATED("use free()")
  PARAOS_INLINE_TRIVIAL
      void Free() noexcept(std::is_nothrow_invocable_v<
                           decltype(&message_type::free), message_type>) {
    free();
  }

 private:
  message_type message_;
  paraos::queue_blocking_base<message_type, QUEUE_SIZE>& queue_;
};

template <
    typename ALLOCATOR = std::allocator<std::uint8_t>,
    const std::size_t QUEUE_SIZE = 1U>
using MessageWritable PARAOS_DEPRECATED("use paraos::message_writable") =
    message_writable<ALLOCATOR, QUEUE_SIZE>;

/// @brief Message buffer base class. Contained API for buffer.
///
/// @tparam BUFFER_ALLOCATOR: Memory allocator for request memory for each
/// message.
/// @tparam QUEUE_SIZE: Max message number for contained in buffer in same
/// time.
template <
    typename BUFFER_ALLOCATOR = std::allocator<std::uint8_t>,
    const std::size_t QUEUE_SIZE = 1U>
class message_buffer_base {
 public:
  virtual ~message_buffer_base() = default;

  message_buffer_base(const message_buffer_base& other) = delete;
  message_buffer_base(message_buffer_base&& other) = delete;
  auto operator=(const message_buffer_base& other)
      -> message_buffer_base& = delete;
  auto operator=(message_buffer_base&& other) -> message_buffer_base& = delete;

  /// @brief  Request memory from allocator, witch set in message_buffer ctor.
  ///
  /// @param[in] size_in_bytes: Requested memory size in bytes.
  ///
  /// @return Return container. Note - container way not contained requested
  /// memory. Befor start any operations with message_writable object, check his
  /// validation (use operator bool).
  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto alloc(std::size_t size_in_bytes)
      const noexcept(std::is_nothrow_invocable_v<
                     message_writable<BUFFER_ALLOCATOR, QUEUE_SIZE>,
                     decltype(size_in_bytes), decltype(queue_)>) {
    return message_writable<BUFFER_ALLOCATOR, QUEUE_SIZE>(
        size_in_bytes, queue_);
  }

  /// @brief Return message container if any data available in buffer.
  ///
  /// @param[in] timeout_ms: Timeout for wait any data in buffer if no data
  /// available in calls time.
  ///
  /// @return Return std::optional object. If no data was read, std::optional
  /// will empty. In otherwise std::optional contained message.
  PARAOS_INLINE_TRIVIAL auto pop(std::size_t timeout_ms) {
    return queue_.pop(timeout_ms);
  }

  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto is_full() const noexcept {
    return queue_.is_full();
  }

  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto is_empty() const noexcept {
    return queue_.is_empty();
  }

  // Backward-compatible deprecated forwarding methods.
  [[nodiscard]] PARAOS_DEPRECATED("use alloc()") PARAOS_INLINE_TRIVIAL
      auto Alloc(std::size_t size_in_bytes) const
      noexcept(std::is_nothrow_invocable_v<
               message_writable<BUFFER_ALLOCATOR, QUEUE_SIZE>,
               decltype(size_in_bytes), decltype(queue_)>) {
    return alloc(size_in_bytes);
  }

  PARAOS_DEPRECATED("use pop()")
  PARAOS_INLINE_TRIVIAL auto Pop(std::size_t timeout_ms) {
    return pop(timeout_ms);
  }

  [[nodiscard]] PARAOS_DEPRECATED("use is_full()") PARAOS_INLINE_TRIVIAL
      auto IsFull() const noexcept {
    return is_full();
  }

  [[nodiscard]] PARAOS_DEPRECATED("use is_empty()") PARAOS_INLINE_TRIVIAL
      auto IsEmpty() const noexcept {
    return is_empty();
  }

 protected:
  explicit message_buffer_base(
      paraos::queue_blocking_base<message<BUFFER_ALLOCATOR>, QUEUE_SIZE>& queue)
      : queue_{queue} {}

 private:
  paraos::queue_blocking_base<message<BUFFER_ALLOCATOR>, QUEUE_SIZE>& queue_;
};

template <
    typename BUFFER_ALLOCATOR = std::allocator<std::uint8_t>,
    const std::size_t QUEUE_SIZE = 1U>
using IMessageBuffer PARAOS_DEPRECATED("use paraos::message_buffer_base") =
    message_buffer_base<BUFFER_ALLOCATOR, QUEUE_SIZE>;

/// @brief Message buffer class.
///
/// @tparam QUEUE_SIZE: Max message numb for contained in buffer in same time.
/// @tparam BUFFER_ALLOCATOR: Memory allocator for request memory for each
/// message.
template <
    const std::size_t QUEUE_SIZE,
    typename BUFFER_ALLOCATOR = std::allocator<std::uint8_t>>
class message_buffer final
    : public message_buffer_base<BUFFER_ALLOCATOR, QUEUE_SIZE> {
  static_assert(
      QUEUE_SIZE > 0, "Message contained counter must be greater then '0'");

 public:
  message_buffer()
      : message_buffer_base<BUFFER_ALLOCATOR, QUEUE_SIZE>{queue_} {}

  ~message_buffer() override = default;

  message_buffer(const message_buffer& other) = delete;
  message_buffer(message_buffer&& other) = delete;
  auto operator=(const message_buffer& other) -> message_buffer& = delete;
  auto operator=(message_buffer&& other) -> message_buffer& = delete;

  explicit operator bool() const { return static_cast<bool>(queue_); }

 private:
  paraos::queue_blocking<message<BUFFER_ALLOCATOR>, QUEUE_SIZE> queue_;
};

template <
    const std::size_t QUEUE_SIZE,
    typename BUFFER_ALLOCATOR = std::allocator<std::uint8_t>>
using MessageBuffer PARAOS_DEPRECATED("use paraos::message_buffer") =
    message_buffer<QUEUE_SIZE, BUFFER_ALLOCATOR>;

}  // namespace paraos

#endif /* PARAOS_MESSAGE_BUFFER_HPP */
