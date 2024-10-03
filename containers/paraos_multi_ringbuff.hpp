/// @file paraos_multi_ringbuff.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
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

#ifndef PARAOS_MULTI_RINGBUFF_HPP
#define PARAOS_MULTI_RINGBUFF_HPP

#include "etl/unordered_map.h"
#include "paraos_attr.h"
#include "paraos_queue_blocking.hpp"
#include "paraos_ringbuff.hpp"

namespace paraos {

class IMultiRingBuff {
  using ringbuff_type = IRingBuff<std::uint8_t>;
  using ringbuff_pointer = ringbuff_type*;

 public:
  virtual ~IMultiRingBuff() = default;

  auto Write(
      std::size_t buff_id, const void* src, std::size_t src_size,
      std::size_t timeout_ms, bool is_isr = false) -> std::size_t {
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    std::size_t written_bytes_numb{0};

    if (buff_id < ring_buff_numb_) {
      auto& bf = ringbuff_[buff_id];
      if (bf->Free() >= src_size) {
        {
          const paraos::CriticalSection critical;
          written_bytes_numb = bf->Write(src, src_size);
        }

        // If ring buffer id not pushed in queue, reader can't read these bytes.
        // Therefore skip written in buffer bytes for free memory.
        if (!queue_.Push(buff_id, timeout_ms)) {
          const paraos::CriticalSection critical;
          bf->Skip(written_bytes_numb);
          written_bytes_numb = 0u;
        }
      }
    }

    return written_bytes_numb;
  }

  auto Write(
      std::size_t buff_id, const gsl::span<std::uint8_t> src,
      std::size_t timeout_ms, bool is_isr = false) -> std::size_t {
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    std::size_t written_bytes_numb{0};

    if (buff_id < ring_buff_numb_) {
      auto& bf = ringbuff_[buff_id];
      if (bf->Free() >= src.size()) {
        {
          const paraos::CriticalSection critical;
          written_bytes_numb = bf->Write(src);
        }

        // If ring buffer id not pushed in queue, reader can't read these bytes.
        // Therefore skip written in buffer bytes for free memory.
        if (!queue_.Push(buff_id, timeout_ms)) {
          const paraos::CriticalSection critical;
          bf->Skip(written_bytes_numb);
          written_bytes_numb = 0u;
        }
      }
    }

    return written_bytes_numb;
  }

  auto Read(
      std::size_t& buff_id, void* dst, std::size_t dst_size,
      std::size_t timeout_ms, bool is_isr = false) -> std::size_t {
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    std::size_t read_bytes_numb{0};
    // queue_.Pop return std::optional
    auto ring_buff_id = queue_.Pop(timeout_ms);
    if (ring_buff_id) {
      const paraos::CriticalSection critical;
      buff_id = *ring_buff_id;
      auto& bf = ringbuff_[buff_id];
      read_bytes_numb = bf->Read(dst, dst_size);
    }

    return read_bytes_numb;
  }

  auto Read(
      std::size_t& buff_id, gsl::span<std::uint8_t> dst, std::size_t timeout_ms,
      bool is_isr = false) -> std::size_t {
    PARAOS_ATTR_UNUSED_VAR(is_isr);

    std::size_t read_bytes_numb{0};
    // queue_.Pop return std::optional
    auto ring_buff_id = queue_.Pop(timeout_ms);
    if (ring_buff_id) {
      const paraos::CriticalSection critical;
      buff_id = *ring_buff_id;
      auto& bf = ringbuff_[buff_id];
      read_bytes_numb = bf->Read(dst);
    }

    return read_bytes_numb;
  }

 protected:
  IMultiRingBuff(
      IQueueBlocking<std::size_t>& queue, ringbuff_pointer* ringbuff,
      std::size_t ring_buff_numb)
      : queue_{queue}, ringbuff_{ringbuff}, ring_buff_numb_{ring_buff_numb} {}

 private:
  IQueueBlocking<std::size_t>& queue_;
  ringbuff_pointer* ringbuff_;
  const std::size_t ring_buff_numb_;
};

template <
    std::size_t QUEUE_SIZE, std::size_t RING_BUFF_SIZE,
    std::size_t RING_BUFF_NUMB>
class MultiRingBuff : public IMultiRingBuff {
  using ringbuff_type = RingBuff<std::uint8_t, RING_BUFF_SIZE>;
  using ringbuff_pointer = ringbuff_type*;

  using iringbuff_type = IRingBuff<std::uint8_t>;
  using iringbuff_pointer = iringbuff_type*;

 public:
  MultiRingBuff() : IMultiRingBuff{queue_, ring_buff_ptr, RING_BUFF_NUMB} {
    // Write address for ringbuff_ access by polymorphic IRingBuff class.
    for (std::size_t i = 0; i < RING_BUFF_NUMB; ++i) {
      ring_buff_ptr[i] = &ringbuff_[i];
    }
  }

  virtual ~MultiRingBuff() = default;

 private:
  QueueBlocking<std::size_t, QUEUE_SIZE> queue_;
  ringbuff_type ringbuff_[RING_BUFF_NUMB];

  /// @brief Array of pointers for polymorphic classes, each element contained
  /// address one ringbuff_ exemplar. Need for using in IMultiRingBuff class and
  /// correctly access for each exemplars of ringbuff_ array by polymorphic
  /// IRingBuff class.
  iringbuff_pointer ring_buff_ptr[RING_BUFF_NUMB];
};

}  // namespace paraos

#endif /* PARAOS_MULTI_RINGBUFF_HPP */
