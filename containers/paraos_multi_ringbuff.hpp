/// @file paraos_multi_ringbuff.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_MULTI_RINGBUFF_HPP
#define PARAOS_MULTI_RINGBUFF_HPP

#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "etl/atomic.h"
#include "paraos_attr.h"
#include "paraos_queue_blocking.hpp"
#include "paraos_ringbuff.hpp"

namespace paraos {

template <typename T, std::size_t QueueSize>
class multi_ring_buff_base {
  using ringbuff_type = ring_buff_base<T>;
  using ringbuff_pointer = ringbuff_type*;

  template <typename It>
  using iterator_category_t =
      typename std::iterator_traits<It>::iterator_category;

 public:
  virtual ~multi_ring_buff_base() = default;

  /// @brief Try write data in buffer without delay. All operations in this
  /// method execute atomically.
  ///
  /// @param[in] buff_id: Ring buffer id for write objects from src.
  /// @param[in] src: Pointer on first object in array.
  /// @param[in] src_elem_numb: Number of elements fo write in ring buffer.
  /// @param[in] is_isr: Set true if call from isr.
  ///
  /// @return True if all data write successful, false in otherwise.
  auto try_write(
      std::size_t buff_id, const T* src, std::size_t src_elem_numb,
      bool is_isr = false) {
    const paraos::critical_section critical;

    if (queue_.is_full() || (buff_id >= ring_buff_numb_)) {
      return paraos::isr_bool{false};
    }

    auto& buffer = ringbuff_[buff_id];
    const auto written_elem_numb =
        buffer->write(src, sizeof(T) * src_elem_numb);

    if (written_elem_numb == 0U) {
      return paraos::isr_bool{false};
    }

    paraos::isr_bool is_write_successful = queue_.try_push(buff_id, is_isr);

    // queue_.try_push() can't return false because we check inside critical
    // section if queue full before push.
    PARAOS_CHECK_ASSERT(static_cast<bool>(is_write_successful));

    return is_write_successful;
  }

  template <
      typename TIterator,
      typename U = std::enable_if_t<std::is_base_of_v<
          std::random_access_iterator_tag, iterator_category_t<TIterator>>>>
  PARAOS_INLINE_TRIVIAL auto try_write(
      std::size_t buff_id, TIterator begin, TIterator end,
      bool is_isr = false) {
    const auto elem_count = std::distance(begin, end);
    PARAOS_CHECK_ASSERT(elem_count >= 0);

    const T* data = (begin == end) ? static_cast<const T*>(nullptr)
                                   : std::addressof(*begin);
    return try_write(
        buff_id, data, static_cast<std::size_t>(elem_count), is_isr);
  }

  PARAOS_INLINE_TRIVIAL auto try_write(
      std::size_t buff_id, gsl::span<const T> src, bool is_isr = false) {
    return try_write(buff_id, src.data(), src.size(), is_isr);
  }

  auto read(
      std::size_t& buff_id, void* dst, std::size_t dst_size,
      std::size_t timeout_ms, bool is_isr = false) -> std::size_t {
    PARAOS_CHECK_ASSERT(dst);
    PARAOS_CHECK_ASSERT(dst_size != 0U);

    std::size_t read_bytes_numb{0};
    // queue_.pop return std::optional
    auto ring_buff_id = queue_.pop(timeout_ms, is_isr);
    if (ring_buff_id) {
      const paraos::critical_section critical;
      buff_id = *ring_buff_id;
      auto& buffer = ringbuff_[buff_id];
      read_bytes_numb = buffer->read(dst, dst_size);

      // If not read all available bytes, push ring buffer id in queue for
      // read remaining bytes in next call read().
      if (buffer->size() != 0U) {
        if (!queue_.try_push(buff_id)) {
          // No space in queue. Set force read flag for read data from
          // buffer without request id from queue.
          is_need_force_read_ = true;
        }
      }
    } else if (is_need_force_read_) {
      read_bytes_numb = try_read(buff_id, dst, dst_size);
    }

    return read_bytes_numb;
  }

  PARAOS_INLINE_TRIVIAL auto read(
      std::size_t& buff_id, gsl::span<std::uint8_t> dst, std::size_t timeout_ms,
      bool is_isr = false) -> std::size_t {
    return read(
        buff_id, static_cast<void*>(dst.data()), dst.size(), timeout_ms,
        is_isr);
  }

  auto try_read(
      std::size_t& buff_id, void* dst, std::size_t dst_size,
      bool is_isr = false) -> std::size_t {
    PARAOS_ATTR_UNUSED_VAR(is_isr);
    std::size_t read_bytes_numb{0};

    bool is_need_force_read{false};

    for (std::size_t i = 0; i < ring_buff_numb_; ++i) {
      auto& buffer = ringbuff_[i];
      const paraos::critical_section critical;
      if (buffer->size() != 0U) {
        buff_id = i;
        read_bytes_numb = buffer->read(dst, dst_size);

        // Read anything from buffer, when read() will calls in next time,
        // check again if any data available.
        is_need_force_read = true;
        break;
      }
    }
    is_need_force_read_ = is_need_force_read;
    return read_bytes_numb;
  }

  auto try_read(std::size_t& buff_id, gsl::span<T> dst, bool is_isr = false) {
    return try_read(buff_id, dst.data(), dst.size(), is_isr);
  }

  [[nodiscard]] virtual auto buffer_count() const -> std::size_t {
    return ring_buff_numb_;
  }

  // Backward-compatible deprecated forwarding methods.
  PARAOS_DEPRECATED("use try_write()")
  PARAOS_INLINE_TRIVIAL auto TryWrite(
      std::size_t buff_id, const T* src, std::size_t src_elem_numb,
      bool is_isr = false) {
    return try_write(buff_id, src, src_elem_numb, is_isr);
  }

  template <
      typename TIterator,
      typename U = std::enable_if_t<std::is_base_of_v<
          std::random_access_iterator_tag, iterator_category_t<TIterator>>>>
  PARAOS_DEPRECATED("use try_write()")
  PARAOS_INLINE_TRIVIAL auto TryWrite(
      std::size_t buff_id, TIterator begin, TIterator end,
      bool is_isr = false) {
    return try_write(buff_id, begin, end, is_isr);
  }

  PARAOS_DEPRECATED("use try_write()")
  PARAOS_INLINE_TRIVIAL auto TryWrite(
      std::size_t buff_id, gsl::span<const T> src, bool is_isr = false) {
    return try_write(buff_id, src, is_isr);
  }

  PARAOS_DEPRECATED("use read()")
  PARAOS_INLINE_TRIVIAL auto Read(
      std::size_t& buff_id, void* dst, std::size_t dst_size,
      std::size_t timeout_ms, bool is_isr = false) -> std::size_t {
    return read(buff_id, dst, dst_size, timeout_ms, is_isr);
  }

  PARAOS_DEPRECATED("use read()")
  PARAOS_INLINE_TRIVIAL auto Read(
      std::size_t& buff_id, gsl::span<std::uint8_t> dst, std::size_t timeout_ms,
      bool is_isr = false) -> std::size_t {
    return read(buff_id, dst, timeout_ms, is_isr);
  }

  PARAOS_DEPRECATED("use try_read()")
  PARAOS_INLINE_TRIVIAL auto TryRead(
      std::size_t& buff_id, void* dst, std::size_t dst_size,
      bool is_isr = false) -> std::size_t {
    return try_read(buff_id, dst, dst_size, is_isr);
  }

  PARAOS_DEPRECATED("use try_read()")
  PARAOS_INLINE_TRIVIAL auto TryRead(
      std::size_t& buff_id, gsl::span<T> dst, bool is_isr = false) {
    return try_read(buff_id, dst, is_isr);
  }

  [[nodiscard]] PARAOS_DEPRECATED("use buffer_count()")
  PARAOS_INLINE_TRIVIAL auto GetBuffNumb() const -> std::size_t {
    return buffer_count();
  }

  multi_ring_buff_base(const multi_ring_buff_base& other) = delete;
  multi_ring_buff_base(multi_ring_buff_base&& other) = delete;
  auto operator=(const multi_ring_buff_base& other)
      -> multi_ring_buff_base& = delete;
  auto operator=(multi_ring_buff_base&& other)
      -> multi_ring_buff_base& = delete;

 protected:
  multi_ring_buff_base(
      paraos::queue_blocking_base<std::size_t, QueueSize>& queue,
      ringbuff_pointer* ringbuff, std::size_t ring_buff_numb)
      : queue_{queue}, ringbuff_{ringbuff}, ring_buff_numb_{ring_buff_numb} {}

 private:
  paraos::queue_blocking_base<std::size_t, QueueSize>& queue_;
  ringbuff_pointer* ringbuff_;
  const std::size_t ring_buff_numb_;
  etl::atomic_bool is_need_force_read_{false};
};

template <typename T, std::size_t QueueSize>
using IMultiRingBuff PARAOS_DEPRECATED("use paraos::multi_ring_buff_base") =
    multi_ring_buff_base<T, QueueSize>;

/// @brief Manage many ring buffers.
///
/// @tparam QueueSize: After any write operations in ring buffer
/// successfully complete (by producer), multi_ring_buff send notify to
/// consumers using blocking queue. QueueSize indicates how many
/// notifications can queued in one time.
/// @tparam T: Type of contained objects in all ring buffers.
/// @tparam ...RingBuffs: Parameter packs, each element contained one ring
/// buffer.
template <std::size_t QueueSize, typename T, typename... RingBuffs>
class multi_ring_buff : public multi_ring_buff_base<T, QueueSize> {
  static constexpr std::size_t ring_buffs_numbs{sizeof...(RingBuffs)};

  using iringbuff_type = ring_buff_base<T>;
  using iringbuff_pointer = iringbuff_type*;

  static_assert(
      QueueSize > 1U,
      "Queue size in multi_ring_buff must be greater then one");

 public:
  constexpr multi_ring_buff()
      : multi_ring_buff_base<T, QueueSize>{
            queue_, &ring_buff_ptr_[0], ring_buffs_numbs} {
    // Copy ring buff addresses from tuple in ring_buff_ptr_.
    set_pointers_on_polymorphic_classes(ringbuff_tuple_);
  }

  ~multi_ring_buff() override = default;

  [[nodiscard]] auto buffer_count() const -> std::size_t override {
    return ring_buffs_numbs;
  }

  multi_ring_buff(multi_ring_buff&& other) = delete;
  auto operator=(multi_ring_buff&& other) -> multi_ring_buff& = delete;
  auto operator=(const multi_ring_buff& other) -> multi_ring_buff& = delete;
  multi_ring_buff(const multi_ring_buff& other) = delete;

 private:
  /// @brief Iterate tuple and store address of each ring buffer.
  template <typename TupleT, std::size_t... Is>
  void set_pointers_on_polymorphic_classes_impl(
      TupleT& tup, std::index_sequence<Is...> index_seq) {
    PARAOS_ATTR_UNUSED_VAR(index_seq);
    (..., (ring_buff_ptr_[Is] = &std::get<Is>(tup)));
  }

  template <typename TupleT>
  void set_pointers_on_polymorphic_classes(TupleT& tup) {
    set_pointers_on_polymorphic_classes_impl(
        tup, std::make_index_sequence<ring_buffs_numbs>{});
  }

 private:
  paraos::queue_blocking<std::size_t, QueueSize> queue_{};

  /// @brief Tuple for contained ring buffers.
  std::tuple<RingBuffs...> ringbuff_tuple_{};

  // NOLINTBEGIN(hicpp-avoid-c-arrays)
  /// @brief Array of pointers for polymorphic classes, each element
  /// contained address one ringbuff_ exemplar. Need for using in
  /// multi_ring_buff_base class and correctly access for each exemplars of
  /// ringbuff_ array by polymorphic ring_buff_base class.
  ///
  /// @note std::array can't be used here because the base class
  /// multi_ring_buff_base is initialized before member objects, and its
  /// constructor needs the address of this storage. A C-style array
  /// provides the required stable address without a constructor call.
  iringbuff_pointer ring_buff_ptr_[ring_buffs_numbs]{};
  // NOLINTEND(hicpp-avoid-c-arrays)

};  // class multi_ring_buff

template <std::size_t QueueSize, typename T, typename... RingBuffs>
using MultiRingBuff PARAOS_DEPRECATED("use paraos::multi_ring_buff") =
    multi_ring_buff<QueueSize, T, RingBuffs...>;

}  // namespace paraos

#endif /* PARAOS_MULTI_RINGBUFF_HPP */
