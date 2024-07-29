#ifndef PARAOS_QUEUE_HPP
#define PARAOS_QUEUE_HPP

#include <cassert>
#include <memory>

#include "paraos_config.hpp"
#include "paraos_mutex.hpp"
#include "paraos_semaphore.hpp"
#include "paraos_trace.hpp"

namespace paraos {

/// @brief Finally with no overhead for heap memory
/// @tparam ActTyfexplicit
template <typename ActTy>
struct Finally {
  ActTy act_;
  explicit Finally(ActTy act) : act_{std::move(act)} {}
  ~Finally() { act_(); }
};

template <typename T>
struct IQueue {
  virtual ~IQueue() = default;

  virtual auto Push(const T& element) -> bool = 0;
  virtual auto Push(T&& element) -> bool = 0;
  virtual auto Pop() -> T = 0;
  virtual auto IsEmpty() -> bool = 0;
  virtual auto IsFull() -> bool = 0;
  virtual auto Size() -> size_t = 0;
  virtual void Erase() = 0;

 protected:
  IQueue() = default;
};

template <typename T, typename ALLOCATOR = std::allocator<T>>
struct Queue : public IQueue<T> {
 private:
  ALLOCATOR allocator_;
  using traits_t1 = std::allocator_traits<decltype(allocator_)>;

  static_assert(
      std::is_nothrow_move_constructible<T>::value,
      "'T' move constructor must be annotated as noexcept");

  static_assert(
      std::is_same_v<T, typename traits_t1::value_type>,
      "Queue item type and allocator type must be same type");

 public:
  Queue(size_t max_elements_numb) : max_elements_numb_{max_elements_numb} {
    if (max_elements_numb > 0) {
      buff_ptr_ = traits_t1::allocate(allocator_, max_elements_numb_);
    }
  }

  virtual ~Queue() {
    Erase();

    if (buff_ptr_) {
      traits_t1::deallocate(allocator_, buff_ptr_, max_elements_numb_);
    }
  };

  Queue(const Queue& other) = delete;
  Queue(Queue&& other) = delete;
  Queue& operator=(const Queue& other) = delete;
  Queue& operator=(Queue&& other) = delete;

  operator bool() const {
    bool is_queue_created{false};
    if (buff_ptr_) {
      is_queue_created = true;
    }

    return is_queue_created;
  }

  template <typename... Args>
  auto EmplaceBack(Args&&... args) -> bool {
    bool is_pushed{false};
    if (!IsFull()) {
      traits_t1::construct(
          allocator_, &buff_ptr_[w_idx_], std::forward<Args>(args)...);
      UpdateWriteIdx();

      is_pushed = true;
    }
    return is_pushed;
  }

  auto Push(T&& item) noexcept(std::is_nothrow_move_constructible<T>::value)
      -> bool override {
    return EmplaceBack(std::move(item));
  }

  auto Push(const T& item) noexcept(
      std::is_nothrow_copy_constructible<T>::value) -> bool override {
    if (!IsFull()) {
      traits_t1::construct(allocator_, &buff_ptr_[w_idx_], item);

      UpdateWriteIdx();

      return true;
    }

    return false;
  }

  auto Pop() -> T override {
    assert(
        !IsEmpty() &&
        "if Pop() is called for an empty queue, the behavior is undefined");

    // Лямбда-функция ниже будет вызвана сразу после оператора return
    Finally pop_from_queue{[&] {
      paraosTRACE_MESSAGE("Call pop() for queue");

      traits_t1::destroy(allocator_, &buff_ptr_[r_idx_]);

      UpdateReadIdx();
    }};

    // После оператора return будет вызвана лямбда-функция выше, которая
    // освободит память в очереди
    return std::move(Front());
  }

  virtual auto IsEmpty() -> bool override {
    bool is_empty{false};
    if (contained_cnt_ == 0) {
      is_empty = true;
    }

    return is_empty;
  };

  virtual PARAOS_INLINE_TRIVIAL auto IsFull() -> bool override {
    if (contained_cnt_ >= max_elements_numb_) {
      return true;
    }
    return false;
  }

  virtual PARAOS_INLINE_TRIVIAL auto Size() -> size_t override {
    return contained_cnt_;
  };

  virtual void Erase() override {
    while (!IsEmpty()) {
      Pop();
    }
  };

 private:
  PARAOS_INLINE_TRIVIAL auto Front() -> T& { return buff_ptr_[r_idx_]; }

  [[nodiscard]] auto CyclicIncrementIdx(size_t idx) {
    ++idx;

    // Индекс не может превышать максимально допустимое количество элементов в
    // очереди.
    assert(!(idx > max_elements_numb_));

    if (idx == max_elements_numb_) {
      idx = 0U;
    }

    return idx;
  }

  PARAOS_INLINE_TRIVIAL void UpdateWriteIdx() {
    w_idx_ = CyclicIncrementIdx(w_idx_);
    ++contained_cnt_;
    assert(contained_cnt_ <= max_elements_numb_);
  }

  PARAOS_INLINE_TRIVIAL void UpdateReadIdx() {
    r_idx_ = CyclicIncrementIdx(r_idx_);
    --contained_cnt_;
    assert(contained_cnt_ <= max_elements_numb_);
  }

  /// @brief Указатель на область памяти хранения данных очереди.
  T* buff_ptr_{nullptr};

  /// @brief Размер очереди.
  const size_t max_elements_numb_{0};

  /// @brief Позиция чтения данных из очереди.
  size_t r_idx_{0};

  /// @brief Позиция записи данных в очередь.
  size_t w_idx_{0};

  /// @brief Количество записанных в очередь объектов.
  size_t contained_cnt_{0};
};

}  // namespace paraos

#endif /* PARAOS_QUEUE_HPP */
