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

#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

#include "paraos_attr.h"
#include "paraos_queue_blocking.hpp"
#include "paraos_ringbuff.hpp"

namespace paraos {

template <typename T>
class IMultiRingBuff {
  using ringbuff_type = IRingBuff<T>;
  using ringbuff_pointer = ringbuff_type*;

  template <typename It>
  using iterator_category_t =
      typename std::iterator_traits<It>::iterator_category;

 public:
  virtual ~IMultiRingBuff() = default;

  /// @brief Try write data in buffer without delay. All operations in this
  /// method execute atomically.
  ///
  /// @param[in] buff_id: Ring buffer id for write objects from src.
  /// @param[in] src: Pointer on first object in array.
  /// @param[in] src_elem_numb: Number of elements fo write in ring buffer.
  /// @param[in] is_isr: Set true if call from isr.
  ///
  /// @return Returned number of written elements.
  auto TryWrite(
      const std::size_t buff_id, const T* src, const std::size_t src_elem_numb,
      bool is_isr = false) -> std::size_t {
    std::size_t written_elem_numb{0};
    const paraos::CriticalSection critical;
    if (!queue_.IsFull()) {
      if (buff_id < ring_buff_numb_) {
        auto& buffer = ringbuff_[buff_id];
        written_elem_numb = buffer->Write(src, sizeof(T) * src_elem_numb);

        if (written_elem_numb > 0U) {
          auto is_pushed = queue_.TryPush(buff_id, is_isr);

          // Reduce compile warning if PARAOS_CHECK_ASSERT() empty macros.
          PARAOS_ATTR_UNUSED_VAR(is_pushed);

          // queue_.Push() can't return false because we check inside critical
          // section if queue full befor push.
          PARAOS_CHECK_ASSERT(static_cast<bool>(is_pushed));
        }
      }
    }

    return written_elem_numb;
  }

  template <
      typename TIterator,
      typename U = std::enable_if_t<std::is_base_of_v<
          std::random_access_iterator_tag, iterator_category_t<TIterator>>>>
  PARAOS_INLINE_TRIVIAL auto TryWrite(
      std::size_t buff_id, TIterator begin, TIterator end,
      bool is_isr = false) {
    return TryWrite(buff_id, begin, std::distance(begin, end), is_isr);
  }

  PARAOS_INLINE_TRIVIAL auto TryWrite(
      std::size_t buff_id, const gsl::span<const T> src, bool is_isr = false)
      -> std::size_t {
    return TryWrite(buff_id, src.data(), src.size(), is_isr);
  }

  auto Read(
      std::size_t& buff_id, void* dst, std::size_t dst_size,
      std::size_t timeout_ms, bool is_isr = false) -> std::size_t {
    PARAOS_CHECK_ASSERT(dst);
    PARAOS_CHECK_ASSERT(dst_size != 0U);

    // todo delete after tests
    is_need_force_read_ = true;

    std::size_t read_bytes_numb{0};
    // queue_.Pop return std::optional
    auto ring_buff_id = queue_.Pop(timeout_ms, is_isr);
    if (ring_buff_id) {
      const paraos::CriticalSection critical;
      buff_id = *ring_buff_id;
      auto& buffer = ringbuff_[buff_id];
      read_bytes_numb = buffer->Read(dst, dst_size);

      // If not read all available bytes, push ring buffer id in queue for
      // read remaining bytes in next call Read().
      if (buffer->Size() != 0U) {
        if (!queue_.TryPush(buff_id)) {
          // No space in queue. Set force read flag for read data from
          // buffer without request id from queue.
          is_need_force_read_ = true;
        }
      }
    } else if (is_need_force_read_) {
      read_bytes_numb = TryRead(buff_id, dst, dst_size);
    }

    return read_bytes_numb;
  }

  PARAOS_INLINE_TRIVIAL auto Read(
      std::size_t& buff_id, gsl::span<std::uint8_t> dst, std::size_t timeout_ms,
      bool is_isr = false) -> std::size_t {
    return Read(
        buff_id, static_cast<void*>(dst.data()), dst.size(), timeout_ms,
        is_isr);
  }

  auto TryRead(
      std::size_t& buff_id, void* dst, std::size_t dst_size,
      bool is_isr = false) -> std::size_t {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    std::size_t read_bytes_numb{0};

    bool is_need_force_read{false};

    for (std::size_t i = 0; i < ring_buff_numb_; ++i) {
      auto& buffer = ringbuff_[i];
      const paraos::CriticalSection critical;
      if (buffer->Size() != 0U) {
        buff_id = i;
        read_bytes_numb = buffer->Read(dst, dst_size);

        // Read anything from buffer, when Read() will calls in next time,
        // check again if any data available.
        is_need_force_read = true;
        break;
      }
    }
    is_need_force_read_ = is_need_force_read;
    return read_bytes_numb;
  }

  auto TryRead(std::size_t& buff_id, gsl::span<T> dst, bool is_isr = false) {
    return TryRead(buff_id, dst.data(), dst.size(), is_isr);
  }

  [[nodiscard]] auto GetBuffNumb() const { return ring_buff_numb_; }

  IMultiRingBuff(IMultiRingBuff&& other) = delete;
  auto operator=(IMultiRingBuff&& other) -> IMultiRingBuff& = delete;
  auto operator=(const IMultiRingBuff& other) -> IMultiRingBuff& = delete;
  IMultiRingBuff(const IMultiRingBuff& other) = delete;

 protected:
  IMultiRingBuff(
      paraos::IQueueBlocking<std::size_t>& queue, ringbuff_pointer* ringbuff,
      std::size_t ring_buff_numb)
      : queue_{queue}, ringbuff_{ringbuff}, ring_buff_numb_{ring_buff_numb} {}

 private:
  paraos::IQueueBlocking<std::size_t>& queue_;
  ringbuff_pointer* ringbuff_;
  const std::size_t ring_buff_numb_;
  etl::atomic_bool is_need_force_read_{false};
};

/// @brief Manage many ring buffers.
///
/// @tparam QUEUE_SIZE: After any write operations in ring buffer
/// successfully complete (by producer), MultiRingBuff send notify to
/// consumers using blocking queue. QUEUE_SIZE indicates how many
/// notifications can queued in one time.
/// @tparam T: Type of contained objects in all ring buffers.
/// @tparam ...RINGBUFF: Parameter packs, each element contained one ring
/// buffer.
template <std::size_t QUEUE_SIZE, typename T, typename... RINGBUFF>
class MultiRingBuff : public IMultiRingBuff<T> {
  static constexpr std::size_t ring_buffs_numbs{sizeof...(RINGBUFF)};

  using ringbuff_type = RingBuff<T, ring_buffs_numbs>;
  using ringbuff_pointer = ringbuff_type*;
  using iringbuff_type = IRingBuff<T>;
  using iringbuff_pointer = iringbuff_type*;

  static_assert(
      QUEUE_SIZE > 1U, "Queue size in MultiRingBuff must be greater then one");

 public:
  constexpr MultiRingBuff()
      : IMultiRingBuff<T>{queue_, &ring_buff_ptr[0], ring_buffs_numbs} {
    // Copy ring buff addresses from tuple in ring_buff_ptr.
    SetPointersOnPolymorphicClasses(ringbuff_tuple_);
  }

  ~MultiRingBuff() override = default;

  [[nodiscard]] constexpr auto GetBuffNumb() const {
    return sizeof...(RINGBUFF);
  }

  MultiRingBuff(MultiRingBuff&& other) = delete;
  auto operator=(MultiRingBuff&& other) -> MultiRingBuff& = delete;
  auto operator=(const MultiRingBuff& other) -> MultiRingBuff& = delete;
  MultiRingBuff(const MultiRingBuff& other) = delete;

 private:
  /// --------------------------------------------------------------------------
  /// Methods below need for iterate tuple.
  /// --------------------------------------------------------------------------

  /// @brief Iterate tuple.
  template <typename D>
  void SetPointerOnPolymorphicRingBuffClass(D& ring_buff, int& idx) {
    ring_buff_ptr[idx++] = &ring_buff;
  }

  /// @brief Iterate tuple.
  template <typename TupleT, std::size_t... Is>
  void SetPointersOnPolymorphicClassesManual(
      TupleT& tup, std::index_sequence<Is...> index_seq) {
    PARAOS_ATTR_UNUSED_VAR(index_seq);
    int idx{0};

    // SetPointerOnPolymorphicRingBuffClass() will calls as many times as
    // there are ring buffers contained in the tuple.
    (SetPointerOnPolymorphicRingBuffClass(std::get<Is>(tup), idx), ...);
  }

  /// @brief Iterate tuple.
  template <typename TupleT, std::size_t TupSize = std::tuple_size_v<TupleT>>
  void SetPointersOnPolymorphicClasses(TupleT& tup) {
    SetPointersOnPolymorphicClassesManual(
        tup, std::make_index_sequence<TupSize>{});
  }

 private:
  paraos::QueueBlocking<std::size_t, QUEUE_SIZE> queue_;

  /// @brief Tuple for contained ring buffers.
  std::tuple<RINGBUFF...> ringbuff_tuple_;

  // NOLINTBEGIN(hicpp-avoid-c-arrays)
  /// @brief Array of pointers for polymorphic classes, each element
  /// contained address one ringbuff_ exemplar. Need for using in
  /// IMultiRingBuff class and correctly access for each exemplars of
  /// ringbuff_ array by polymorphic IRingBuff class.
  ///
  /// @note Using C style array, because std::array stays uninitialized in
  /// MultiRingBuff constructor.
  iringbuff_pointer ring_buff_ptr[ring_buffs_numbs];
  // NOLINTEND(hicpp-avoid-c-arrays)
};

}  // namespace paraos

#endif /* PARAOS_MULTI_RINGBUFF_HPP */
