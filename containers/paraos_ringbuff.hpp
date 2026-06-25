/// @file paraos_ringbuff.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_RINGBUFF_HPP
#define PARAOS_RINGBUFF_HPP

#include <cstdint>
#include <iterator>
#include <memory>

#include "etl/error_handler.h"
#include "gsl/gsl"
#include "lwrb/lwrb.h"
#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_config.hpp"
#include "paraos_exceptions.hpp"

namespace paraos {

#define RINGBUFF_FILE_ID ("100")

/// @brief The base class for ring buffer exceptions.
class ring_buff_exception : public paraos::exception {
 public:
  ring_buff_exception(
      error_string_type reason_, error_string_type file_name_,
      error_numeric_type line_number_)
      : exception(reason_, file_name_, line_number_) {}

  ~ring_buff_exception() override = default;

  ring_buff_exception(const ring_buff_exception&) = default;
  auto operator=(const ring_buff_exception&) -> ring_buff_exception& = default;
  ring_buff_exception(ring_buff_exception&&) = default;
  auto operator=(ring_buff_exception&&) -> ring_buff_exception& = default;
};

using ringbuff_exception PARAOS_DEPRECATED("use paraos::ring_buff_exception") =
    ring_buff_exception;

/// @brief Exception may be thrown when error in <ring_buff_base> constructor
/// appears.
class ring_buff_ctor_error_exception final : public ring_buff_exception {
 public:
  ring_buff_ctor_error_exception(
      error_string_type file_name_, error_numeric_type line_number_)
      : paraos::ring_buff_exception(
            paraos::get_error_text("ringbuff:Ctor", RINGBUFF_FILE_ID),
            file_name_, line_number_) {}

  ~ring_buff_ctor_error_exception() override = default;

  ring_buff_ctor_error_exception(const ring_buff_ctor_error_exception&) =
      default;
  auto operator=(const ring_buff_ctor_error_exception&)
      -> ring_buff_ctor_error_exception& = default;
  ring_buff_ctor_error_exception(ring_buff_ctor_error_exception&&) = default;
  auto operator=(ring_buff_ctor_error_exception&&)
      -> ring_buff_ctor_error_exception& = default;
};

using ringbuff_ctor_error_exception PARAOS_DEPRECATED(
    "use paraos::ring_buff_ctor_error_exception") =
    ring_buff_ctor_error_exception;

/// @brief  This is the base for all ring buffers that contain a particular
/// type.
/// @details Normally a reference to this type will be taken from a derived
/// ring_buff.
/// @tparam T: Type elements, contained in ring buffer.
template <typename T>
class ring_buff_base {
  using value_type = T;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using reference = value_type&;
  using const_reference = const value_type&;
  using iterator = value_type*;
  using const_iterator = const value_type*;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  template <typename It>
  using iterator_category_t =
      typename std::iterator_traits<It>::iterator_category;

 public:
  virtual ~ring_buff_base() = default;

  explicit operator bool() const {
    return static_cast<bool>(lwrb_is_ready(const_cast<lwrb_t*>(&lwrb_)));
  }

  PARAOS_INLINE_TRIVIAL auto write(const void* src, lwrb_sz_t size_in_bytes) {
    lwrb_sz_t written{0};
    lwrb_write_ex(&lwrb_, src, size_in_bytes, &written, LWRB_FLAG_WRITE_ALL);
    return written;
  }

  PARAOS_INLINE_TRIVIAL auto write(gsl::span<const T> src) {
    return write(static_cast<const void*>(src.data()), src.size_bytes());
  }

  template <
      typename TIterator,
      typename U = std::enable_if_t<std::is_base_of_v<
          std::random_access_iterator_tag, iterator_category_t<TIterator>>>>
  PARAOS_INLINE_TRIVIAL auto write(TIterator begin, TIterator end) {
    const auto elem_count = std::distance(begin, end);
    PARAOS_CHECK_ASSERT(elem_count >= 0);

    const T* data = (begin == end) ? static_cast<const T*>(nullptr)
                                   : std::addressof(*begin);
    return write(
        static_cast<const void*>(data),
        static_cast<lwrb_sz_t>(
            static_cast<std::size_t>(elem_count) * sizeof(T)));
  }

  PARAOS_INLINE_TRIVIAL auto read(void* dst, lwrb_sz_t dst_size_in_bytes) {
    return lwrb_read(&lwrb_, dst, dst_size_in_bytes);
  }

  PARAOS_INLINE_TRIVIAL auto read(gsl::span<T> dst) {
    return read(
        static_cast<void*>(dst.data()),
        static_cast<lwrb_sz_t>(dst.size_bytes()));
  }

  PARAOS_INLINE_TRIVIAL auto peek(void* dst, lwrb_sz_t dst_size_in_bytes) {
    // how many bytes need skip before peek data from ring buff.
    constexpr lwrb_sz_t skip_count{0};
    return lwrb_peek(&lwrb_, skip_count, dst, dst_size_in_bytes);
  }

  PARAOS_INLINE_TRIVIAL auto peek(gsl::span<T> dst) {
    return peek(
        static_cast<void*>(dst.data()),
        static_cast<lwrb_sz_t>(dst.size_bytes()));
  }

  PARAOS_INLINE_TRIVIAL auto skip(lwrb_sz_t size_in_bytes) {
    return lwrb_skip(&lwrb_, size_in_bytes);
  }

  /// @brief Return how many elements can be written.
  ///
  /// @return How many elements can be written before buffer be full.
  [[nodiscard]] PARAOS_INLINE_TRIVIAL auto free_space() const {
    return lwrb_get_free(&lwrb_);
  }

  /// @brief Return numbers of bytes currently available in buffer.
  /// @return Number of bytes ready to be read
  [[nodiscard]] auto size() const { return lwrb_get_full(&lwrb_); }

  /// @brief Return how many elements of T type buffer can contained in each
  /// time.
  ///
  /// @note lwrb buff can contained 'size - 1' bytes numb
  ///
  /// @return Buffer capacity in 'T' object type.
  [[nodiscard]] auto capacity() const { return lwrb_.size - 1; }

  /// @brief Reset ring buffer is inital state. Invalidate all data in ring
  /// buffer.
  void clear() { lwrb_reset(&lwrb_); }

  /// @brief Check is buffer empty. If buffer empty, thats mean user code can
  /// write elements numb, equal capacity().
  ///
  /// @return Return tue if buffer empty, false in otherwise.
  [[nodiscard]] auto is_empty() const -> bool { return size() == 0; }

  /// @brief Check is buffer full. If full, thats mean user code must read or
  /// clear() buffer before write anything again.
  ///
  /// @return Return full if buffer is full, false in otherwise.
  [[nodiscard]] auto is_full() const -> bool { return size() == capacity(); }

  // Backward-compatible deprecated forwarding methods.
  PARAOS_DEPRECATED("use write()")
  PARAOS_INLINE_TRIVIAL auto Write(const void* src, lwrb_sz_t size_in_bytes) {
    return write(src, size_in_bytes);
  }

  PARAOS_DEPRECATED("use write()")
  PARAOS_INLINE_TRIVIAL auto Write(gsl::span<const T> src) {
    return write(src);
  }

  template <
      typename TIterator,
      typename U = std::enable_if_t<std::is_base_of_v<
          std::random_access_iterator_tag, iterator_category_t<TIterator>>>>
  PARAOS_DEPRECATED("use write()")
  PARAOS_INLINE_TRIVIAL auto Write(TIterator begin, TIterator end) {
    return write(begin, end);
  }

  PARAOS_DEPRECATED("use read()")
  PARAOS_INLINE_TRIVIAL auto Read(void* dst, lwrb_sz_t dst_size_in_bytes) {
    return read(dst, dst_size_in_bytes);
  }

  PARAOS_DEPRECATED("use read()")
  PARAOS_INLINE_TRIVIAL auto Read(gsl::span<T> dst) { return read(dst); }

  PARAOS_DEPRECATED("use peek()")
  PARAOS_INLINE_TRIVIAL auto Peek(void* dst, lwrb_sz_t dst_size_in_bytes) {
    return peek(dst, dst_size_in_bytes);
  }

  PARAOS_DEPRECATED("use peek()")
  PARAOS_INLINE_TRIVIAL auto Peek(gsl::span<T> dst) { return peek(dst); }

  PARAOS_DEPRECATED("use skip()")
  PARAOS_INLINE_TRIVIAL auto Skip(lwrb_sz_t size_in_bytes) {
    return skip(size_in_bytes);
  }

  [[nodiscard]] PARAOS_DEPRECATED("use free_space()") PARAOS_INLINE_TRIVIAL
      auto Free() const {
    return free_space();
  }

  [[nodiscard]] PARAOS_DEPRECATED("use size()") auto Size() const {
    return size();
  }

  [[nodiscard]] PARAOS_DEPRECATED("use capacity()") auto Capacity() const {
    return capacity();
  }

  PARAOS_DEPRECATED("use clear()") void Clear() { clear(); }

  [[nodiscard]] PARAOS_DEPRECATED("use is_empty()") auto IsEmpty() const
      -> bool {
    return is_empty();
  }

  [[nodiscard]] PARAOS_DEPRECATED("use is_full()") auto IsFull() const -> bool {
    return is_full();
  }

  /// @brief Five rule.
  ring_buff_base(const ring_buff_base& other) = delete;
  ring_buff_base(ring_buff_base&& other) = delete;
  auto operator=(const ring_buff_base& other) -> ring_buff_base& = delete;
  auto operator=(ring_buff_base&& other) -> ring_buff_base& = delete;

 protected:
  ring_buff_base(void* buff, lwrb_sz_t buff_size_in_bytes) {
    auto is_init_success = lwrb_init(&lwrb_, buff, buff_size_in_bytes);

    ETL_ASSERT(
        is_init_success == 1U, ETL_ERROR(ring_buff_ctor_error_exception));

    PARAOS_ATTR_UNUSED_VAR(is_init_success);
  }

 private:
  lwrb_t lwrb_{};
};

template <typename T>
using IRingBuff PARAOS_DEPRECATED("use paraos::ring_buff_base") =
    ring_buff_base<T>;

/// @brief A fixed capacity ring buffer.
/// @tparam T: Type of contained object. Use <std::uint8_t>
/// @tparam SIZE: Capacity of the ring buffer. Total capacity in bytes will
/// 'SIZE * sizeof(T)'
template <typename T, std::size_t SIZE>
class ring_buff : public ring_buff_base<T> {
  static_assert(
      SIZE > 1, "Size of ring buffer must be greater than one element");

 public:
  /// @brief Construct ring buff wrapper over lwrb.
  ///
  /// @note lwrb need one more byte.
  ring_buff() : ring_buff_base<T>(static_cast<void*>(storage), SIZE + 1) {}

 private:
  // NOLINTBEGIN(hicpp-avoid-c-arrays)
  /// @brief Use raw array for contained bytes in ring buff. If we used
  /// std::array, when call ring_buff_base ctor std::array will not initialized
  /// yet. For this reason used raw array.
  ///
  /// @note lwrb need one more byte.
  T storage[SIZE + 1];
  // NOLINTEND(hicpp-avoid-c-arrays)
};

template <typename T, std::size_t SIZE>
using RingBuff PARAOS_DEPRECATED("use paraos::ring_buff") = ring_buff<T, SIZE>;

}  // namespace paraos

#endif /* PARAOS_RINGBUFF_HPP */
