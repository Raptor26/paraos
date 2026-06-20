/// @file paraos_ringbuff.hpp
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
class ringbuff_exception : public paraos::exception {
 public:
  ringbuff_exception(
      error_string_type reason_, error_string_type file_name_,
      numeric_type line_number_)
      : exception(reason_, file_name_, line_number_) {}

  ~ringbuff_exception() override = default;

  ringbuff_exception(const ringbuff_exception&) = default;
  auto operator=(const ringbuff_exception&) -> ringbuff_exception& = default;
  ringbuff_exception(ringbuff_exception&&) = default;
  auto operator=(ringbuff_exception&&) -> ringbuff_exception& = default;
};

/// @brief Exception may be thrown when error in <IRingBuff> constructor
/// appears.
class ringbuff_ctor_error_exception final : public ringbuff_exception {
 public:
  ringbuff_ctor_error_exception(
      error_string_type file_name_, numeric_type line_number_)
      : paraos::ringbuff_exception(
            paraos::GetErrorText("ringbuff:Ctor", RINGBUFF_FILE_ID), file_name_,
            line_number_) {}

  ~ringbuff_ctor_error_exception() override = default;

  ringbuff_ctor_error_exception(const ringbuff_ctor_error_exception&) = default;
  auto operator=(const ringbuff_ctor_error_exception&)
      -> ringbuff_ctor_error_exception& = default;
  ringbuff_ctor_error_exception(ringbuff_ctor_error_exception&&) = default;
  auto operator=(ringbuff_ctor_error_exception&&)
      -> ringbuff_ctor_error_exception& = default;
};

/// @brief  This is the base for all ring buffers that contain a particular
/// type.
/// @details Normally a reference to this type will be taken from a derived
/// RingBuff.
/// @tparam T: Type elements, contained in ring buffer.
template <typename T>
class IRingBuff {
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
  virtual ~IRingBuff() = default;

  explicit operator bool() const {
    return static_cast<bool>(lwrb_is_ready(&lwrb_));
  }

  PARAOS_INLINE_TRIVIAL auto Write(const void* src, lwrb_sz_t size_in_bytes) {
    lwrb_sz_t written{0};
    lwrb_write_ex(&lwrb_, src, size_in_bytes, &written, LWRB_FLAG_WRITE_ALL);
    return written;
  }

  PARAOS_INLINE_TRIVIAL auto Write(gsl::span<const T> src) {
    return Write(static_cast<const void*>(src.data()), src.size_bytes());
  }

  template <
      typename TIterator,
      typename U = std::enable_if_t<std::is_base_of_v<
          std::random_access_iterator_tag, iterator_category_t<TIterator>>>>
  PARAOS_INLINE_TRIVIAL auto Write(TIterator begin, TIterator end) {
    const auto elem_count = std::distance(begin, end);
    PARAOS_CHECK_ASSERT(elem_count >= 0);

    const T* data = (begin == end) ? static_cast<const T*>(nullptr)
                                   : std::addressof(*begin);
    return Write(
        static_cast<const void*>(data),
        static_cast<lwrb_sz_t>(
            static_cast<std::size_t>(elem_count) * sizeof(T)));
  }

  PARAOS_INLINE_TRIVIAL auto Read(void* dst, lwrb_sz_t dst_size_in_bytes) {
    return lwrb_read(&lwrb_, dst, dst_size_in_bytes);
  }

  PARAOS_INLINE_TRIVIAL auto Read(gsl::span<T> dst) {
    return Read(
        static_cast<void*>(dst.data()),
        static_cast<lwrb_sz_t>(dst.size_bytes()));
  }

  PARAOS_INLINE_TRIVIAL auto Peek(void* dst, lwrb_sz_t dst_size_in_bytes) {
    // how many bytes need skip before peek data from ring buff.
    constexpr lwrb_sz_t skip_count{0};
    return lwrb_peek(&lwrb_, skip_count, dst, dst_size_in_bytes);
  }

  PARAOS_INLINE_TRIVIAL auto Peek(gsl::span<T> dst) {
    return Peek(
        static_cast<void*>(dst.data()),
        static_cast<lwrb_sz_t>(dst.size_bytes()));
  }

  PARAOS_INLINE_TRIVIAL auto Skip(lwrb_sz_t size_in_bytes) {
    return lwrb_skip(&lwrb_, size_in_bytes);
  }

  /// @brief Return how many elements can be written.
  ///
  /// @return How many elements can be written before buffer be full.
  PARAOS_INLINE_TRIVIAL auto Free() const { return lwrb_get_free(&lwrb_); }

  /// @brief Return numbers of bytes currently available in buffer.
  /// @return Number of bytes ready to be read
  auto Size() const { return lwrb_get_full(&lwrb_); }

  /// @brief Return how many elements of T type buffer can contained in each
  /// time.
  ///
  /// @note lwrb buff can contained 'size - 1' bytes numb
  ///
  /// @return Buffer capacity in 'T' object type.
  auto Capacity() const { return lwrb_.size - 1; }

  /// @brief Reset ring buffer is inital state. Invalidate all data in ring
  /// buffer.
  void Clear() { lwrb_reset(&lwrb_); }

  /// @brief Check is buffer empty. If buffer empty, thats mean user code can
  /// write elements numb, equal Capacity().
  ///
  /// @return Return tue if buffer empty, false in otherwise.
  auto IsEmpty() const -> bool { return Size() == 0; }

  /// @brief Check is buffer full. If full, thats mean user code must read or
  /// Clear() buffer before write anything again.
  ///
  /// @return Return full if buffer is full, false in otherwise.
  auto IsFull() const -> bool { return Size() == Capacity(); }

  /// @brief Five rule.
  IRingBuff(const IRingBuff& other) = delete;
  IRingBuff(IRingBuff&& other) = delete;
  auto operator=(const IRingBuff& other) -> IRingBuff& = delete;
  auto operator=(IRingBuff&& other) -> IRingBuff& = delete;

 protected:
  IRingBuff(void* buff, lwrb_sz_t buff_size_in_bytes) {
    auto is_init_success = lwrb_init(&lwrb_, buff, buff_size_in_bytes);

    ETL_ASSERT(is_init_success == 1U, ETL_ERROR(ringbuff_ctor_error_exception));

    PARAOS_ATTR_UNUSED_VAR(is_init_success);
  }

 private:
  lwrb_t lwrb_{};
};

/// @brief A fixed capacity ring buffer.
/// @tparam T: Type of contained object. Use <std::uint8_t>
/// @tparam SIZE: Capacity of the ring buffer. Total capacity in bytes will
/// 'SIZE * sizeof(T)'
template <typename T, std::size_t SIZE>
class RingBuff : public IRingBuff<T> {
  static_assert(
      SIZE > 1, "Size of ring buffer must be greater than one element");

 public:
  /// @brief Construct ring buff wrapper over lwrb.
  ///
  /// @note lwrb need one more byte.
  RingBuff() : IRingBuff<T>(static_cast<void*>(storage), SIZE + 1) {}

 private:
  // NOLINTBEGIN(hicpp-avoid-c-arrays)
  /// @brief Use raw array for contained bytes in ring buff. If we used
  /// std::array, when call IRingBuff ctor std::array will not initialized yet.
  /// For this reason used raw array.
  ///
  /// @note lwrb need one more byte.
  T storage[SIZE + 1];
  // NOLINTEND(hicpp-avoid-c-arrays)
};

}  // namespace paraos

#endif /* PARAOS_RINGBUFF_HPP */
