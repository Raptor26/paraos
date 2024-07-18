#ifndef PARAOS_QUEUE_HPP
#define PARAOS_QUEUE_HPP

#include <deque>
#include <memory>
#include <vector>

namespace paraos {

/// @brief Todo используй std::vector для хранения элементов.
/// @tparam T
template <typename T>
struct Queue {
  Queue(size_t max_elements_numb) {}

  virtual ~Queue() = default;

  auto Push(const T &item) { return false; }

 private:
  /// @brief Вектор для хранения элементов очереди.
  std::deque<T> buff_;

  /// @brief Размер очереди.
  size_t max_elements_numb_{0};

  /// @brief Позиция чтения данных из очереди.
  size_t r_idx_{0};

  /// @brief Позиция записи данных в очередь.
  size_t w_idx_{0};
};

}  // namespace paraos

#endif /* PARAOS_QUEUE_HPP */
