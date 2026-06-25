/// @file paraos_testing_semaphore.hpp
/// @author Vyhodcev Egor (vyhodcev@internet.ru)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_TESTING_SEMAPHORE_HPP
#define PARAOS_TESTING_SEMAPHORE_HPP

#include <cstdint>
#include <utility>

#include "etl/atomic.h"
#include "paraos_attr.h"
#include "paraos_isr.hpp"

namespace paraos {

/// @brief Semaphore attributes for it's initialization.
struct testing_semaphore_attr {
  /// @brief Maximum number of threads that can execute the code protected by
  /// semaphore at a time.
  size_t max_count{1U};

  /// @brief Semaphore counter initial value.
  size_t initial_count{0U};
};
using TestingSemaphoreAttr PARAOS_DEPRECATED(
    "use paraos::testing_semaphore_attr") = testing_semaphore_attr;

/// @brief Testing semaphore class.
///
/// @note This class methods are non-blocking.
class testing_semaphore {
 public:
  /// @brief Testing semaphore constructor.
  /// @param[in] attrs: Attributes for class initialization.
  explicit testing_semaphore(const testing_semaphore_attr& attrs)
      : semaphore_counter_{attrs.initial_count}, max_count_{attrs.max_count} {
    if ((max_count_ > 0) && (semaphore_counter_ <= max_count_)) {
      is_init_succeeded_ = true;
    }
  }

  /// @brief Move ctor.
  testing_semaphore(testing_semaphore&& other) noexcept {
    if (this != &other) {
      this->is_init_succeeded_ = other.is_init_succeeded_;
      this->semaphore_counter_ = other.semaphore_counter_.load();
      this->max_count_ = other.max_count_.load();
    }
  }

  /// @brief Move assignment.
  auto operator=(testing_semaphore&& other) noexcept -> testing_semaphore& {
    if (this != &other) {
      this->~testing_semaphore();
      this->is_init_succeeded_ = other.is_init_succeeded_;
      this->semaphore_counter_ = other.semaphore_counter_.load();
      this->max_count_ = other.max_count_.load();
    }

    return *this;
  }

  /// @brief Semaphore non-copyable
  testing_semaphore(const testing_semaphore& other) = delete;
  auto operator=(const testing_semaphore& other) -> testing_semaphore& = delete;

  virtual ~testing_semaphore() = default;

  /// @brief Take semaphore.
  ///
  /// @param[in] timeout_ms: Not used in current realization.
  ///
  /// @param[in] from_isr: Not used in current realization.
  ///
  /// @return Return isr_bool with true state if semaphore counter was greater
  /// than zero, false state in otherwise.
  auto take(std::size_t timeout_ms = 0, bool from_isr = false) -> isr_bool {
    PARAOS_ATTR_UNUSED_VAR(timeout_ms);
    PARAOS_ATTR_UNUSED_VAR(from_isr);

    bool take_result{false};

    if (semaphore_counter_ != 0) {
      semaphore_counter_--;
      take_result = true;
    }

    return isr_bool{take_result};
  }

  /// @brief Release semaphore.
  ///
  /// @param[in] from_isr: Not used in current realization.
  ///
  /// @return Return operation status. isr_bool with true state if semaphore
  /// counter was less than max count, otherwise - false.
  auto give(bool from_isr = false) -> isr_bool {
    PARAOS_ATTR_UNUSED_VAR(from_isr);

    bool give_result{true};

    ++semaphore_counter_;

    if (semaphore_counter_ > max_count_) {
      semaphore_counter_.store(max_count_);
      give_result = false;
    }

    return isr_bool{give_result};
  }

  /// @brief Backward-compatible deprecated wrapper for take().
  PARAOS_DEPRECATED("use take()")
  auto Take(std::size_t timeout_ms = 0, bool from_isr = false) -> isr_bool {
    return take(timeout_ms, from_isr);
  }

  /// @brief Backward-compatible deprecated wrapper for give().
  PARAOS_DEPRECATED("use give()") auto Give(bool from_isr = false) -> isr_bool {
    return give(from_isr);
  }

  explicit operator bool() const { return is_init_succeeded_; }

 protected:
  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  // We can't put these variables into private section, because they're used in
  // derived classes
  etl::atomic<std::size_t> semaphore_counter_;

  etl::atomic<std::size_t> max_count_;

  bool is_init_succeeded_{false};
  // NOLINTEND(misc-non-private-member-variables-in-classes)
};
using TestingSemaphore PARAOS_DEPRECATED("use paraos::testing_semaphore") =
    testing_semaphore;

/// @brief Binary semaphore class.
///
/// @note Only one thread can execute the code protected with binary semaphore.
class binary_testing_semaphore final : public testing_semaphore {
 public:
  /// @brief Binary semaphore constructor.
  binary_testing_semaphore()
      : binary_testing_semaphore{testing_semaphore_attr{}} {}

  /// @brief Move ctor.
  binary_testing_semaphore(binary_testing_semaphore&& other) noexcept
      : testing_semaphore{std::move(other)} {}

  /// @brief Move assignment.
  auto operator=(binary_testing_semaphore&& other) noexcept
      -> binary_testing_semaphore& {
    if (this != &other) {
      this->~binary_testing_semaphore();
      this->is_init_succeeded_ = other.is_init_succeeded_;
      this->semaphore_counter_ = other.semaphore_counter_.load();
      this->max_count_ = other.max_count_.load();
    }

    return *this;
  }

  /// @brief Semaphore non-copyable
  binary_testing_semaphore(const binary_testing_semaphore& other) = delete;
  auto operator=(const binary_testing_semaphore& other)
      -> binary_testing_semaphore& = delete;

  ~binary_testing_semaphore() override = default;

 private:
  explicit binary_testing_semaphore(const testing_semaphore_attr& attrs)
      : testing_semaphore{attrs} {}
};
using BinaryTestingSemaphore PARAOS_DEPRECATED(
    "use paraos::binary_testing_semaphore") = binary_testing_semaphore;

/// @brief Testing semaphore class which always returns true inside it's
/// "take()" and "give()" methods.
class always_true_semaphore final {
 public:
  always_true_semaphore() = default;

  /// @brief Take semaphore.
  ///
  /// @param[in] timeout_ms: Not used in current realization.
  ///
  /// @param[in] from_isr: Not used in current realization.
  ///
  /// @return Returns true every time.
  static auto take(std::size_t timeout_ms = 0, bool from_isr = false)
      -> isr_bool {
    PARAOS_ATTR_UNUSED_VAR(timeout_ms);
    PARAOS_ATTR_UNUSED_VAR(from_isr);

    return isr_bool{true};
  }

  /// @brief Release semaphore.
  ///
  /// @param[in] from_isr: Not used in current realization.
  ///
  /// @return Returns true every time.
  static auto give(bool from_isr = false) -> isr_bool {
    PARAOS_ATTR_UNUSED_VAR(from_isr);

    return isr_bool{true};
  }

  /// @brief Backward-compatible deprecated wrapper for take().
  PARAOS_DEPRECATED("use take()")
  static auto Take(std::size_t timeout_ms = 0, bool from_isr = false)
      -> isr_bool {
    return take(timeout_ms, from_isr);
  }

  /// @brief Backward-compatible deprecated wrapper for give().
  PARAOS_DEPRECATED("use give()")
  static auto Give(bool from_isr = false) -> isr_bool {
    return give(from_isr);
  }

  /// @brief Move ctor.
  always_true_semaphore(always_true_semaphore&& other) noexcept {
    if (this != &other) {
      this->is_init_succeeded_ = other.is_init_succeeded_;
    }
  }

  /// @brief Move assignment.
  auto operator=(always_true_semaphore&& other) noexcept
      -> always_true_semaphore& {
    if (this != &other) {
      this->~always_true_semaphore();
      this->is_init_succeeded_ = other.is_init_succeeded_;
    }

    return *this;
  }

  /// @brief Semaphore non-copyable
  always_true_semaphore(const always_true_semaphore& other) = delete;
  auto operator=(const always_true_semaphore& other)
      -> always_true_semaphore& = delete;

  ~always_true_semaphore() = default;

  explicit operator bool() const { return is_init_succeeded_; }

 private:
  bool is_init_succeeded_{true};
};
using AlwaysTrueSemaphore PARAOS_DEPRECATED(
    "use paraos::always_true_semaphore") = always_true_semaphore;

}  // namespace paraos

#endif /* PARAOS_TESTING_SEMAPHORE_HPP */
