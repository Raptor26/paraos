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

#include "etl/error_handler.h"
#include "etl/exception.h"
#include "gsl/gsl"
#include "lwrb/lwrb.h"
#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_config.hpp"

namespace paraos {

#define RINGBUFF_FILE_ID (100)

/// The base class for ring buffer exceptions.
class ringbuff_exception : public etl::exception {
 public:
  ringbuff_exception(
      string_type reason_, string_type file_name_, numeric_type line_number_)
      : exception(reason_, file_name_, line_number_) {}
};

class ringbuff_ctor_error : public ringbuff_exception {
 public:
  ringbuff_ctor_error(string_type file_name_, numeric_type line_number_)
      : ringbuff_exception(
            ETL_ERROR_TEXT("ringbuff:Ctor", RINGBUFF_FILE_ID), file_name_,
            line_number_) {}
};

/// @brief  This is the base for all ring buffers that contain a particular
/// type.
///@details Normally a reference to this type will be taken from a derived
/// RingBuff.
/// @tparam T: Type elements, contained in ring buffer.
template <typename T>
class IRingBuff {
  typedef T value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* iterator;
  typedef const value_type* const_iterator;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

 public:
  virtual ~IRingBuff() = default;

  operator bool() { return lwrb_is_ready(&lwrb_); }

  PARAOS_INLINE_TRIVIAL auto Write(const void* src, lwrb_sz_t size_in_bytes) {
    lwrb_sz_t written{0};
    lwrb_write_ex(&lwrb_, src, size_in_bytes, &written, LWRB_FLAG_WRITE_ALL);
    return written;
  }

  PARAOS_INLINE_TRIVIAL auto Write(const gsl::span<const T> src) {
    return Write(static_cast<const void*>(src.data()), src.size_bytes());
  }

  template <class TIterator>
  PARAOS_INLINE_TRIVIAL auto Write(TIterator begin, TIterator end) {
    return Write(
        static_cast<const void*>(begin),
        static_cast<lwrb_sz_t>(std::distance(begin, end)));
  }

  PARAOS_INLINE_TRIVIAL auto Read(void* dst, lwrb_sz_t dst_size_in_bytes) {
    return lwrb_read(&lwrb_, dst, dst_size_in_bytes);
  }

  PARAOS_INLINE_TRIVIAL auto Read(gsl::span<T> dst) {
    return Read(
        static_cast<void*>(dst.data()),
        static_cast<lwrb_sz_t>(dst.size_bytes()));
  }

  PARAOS_INLINE_TRIVIAL auto Skip(lwrb_sz_t size_in_bytes) {
    return lwrb_skip(&lwrb_, size_in_bytes);
  }

  PARAOS_INLINE_TRIVIAL auto Free() { return lwrb_get_free(&lwrb_); }

  /// @brief Return numbers of bytes currently available in buffer.
  /// @return Number of bytes ready to be read
  auto Size() { return lwrb_get_full(&lwrb_); }

  /// @brief Reset ring buffer is inital state. Invalidate all data in ring
  /// buffer.
  void Clear() { lwrb_reset(&lwrb_); }

 protected:
  IRingBuff(void* buff, lwrb_sz_t buff_size_in_bytes) {
    auto is_init_success = lwrb_init(&lwrb_, buff, buff_size_in_bytes);

    ETL_ASSERT(is_init_success == 1u, ETL_ERROR(ringbuff_ctor_error));

    PARAOS_ATTR_UNUSED_VAR(is_init_success);
  }

 private:
  lwrb_t lwrb_;
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
  RingBuff() : IRingBuff<T>(static_cast<void*>(storage), SIZE) {}

 private:
  /// @brief Use raw array for contained bytes in ring buff. If we used
  /// std::array, when call IRingBuff ctor std::array will not initialized yet.
  /// For this reason used raw array.
  T storage[SIZE];
};

}  // namespace paraos

#endif /* PARAOS_RINGBUFF_HPP */
