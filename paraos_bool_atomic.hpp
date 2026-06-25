/// @file paraos_bool_atomic.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_BOOL_ATOMIC_HPP
#define PARAOS_BOOL_ATOMIC_HPP

#include <utility>

#include "paraos_config.hpp"
#include "paraos_critical.hpp"

namespace paraos {

template <typename T>
class var_atomic final {
 public:
  explicit var_atomic(T var) {
    const paraos::critical_section critical;  // RAII
    var_ = var;
  }

  ~var_atomic() = default;

  // ---------------------------------------------------------------------------
  // Five Rule
  // ---------------------------------------------------------------------------

  PARAOS_INLINE_TRIVIAL var_atomic(const var_atomic& other) noexcept {
    const paraos::critical_section critical;  // RAII
    var_ = other.var_;
  }

  PARAOS_INLINE_TRIVIAL var_atomic(var_atomic&& other) noexcept {
    const paraos::critical_section critical;  // RAII
    var_ = std::move(other.var_);
  }

  PARAOS_INLINE_TRIVIAL auto operator=(const var_atomic& other) noexcept
      -> var_atomic& {
    if (this != &other) {
      const paraos::critical_section critical;  // RAII
      var_ = other.var_;
    }

    return *this;
  }

  PARAOS_INLINE_TRIVIAL auto operator=(var_atomic&& other) noexcept
      -> var_atomic& {
    const paraos::critical_section critical;  // RAII
    var_ = std::move(other.var_);

    return *this;
  }
  // ---------------------------------------------------------------------------

  explicit operator bool() const {
    static_assert(
        std::is_same_v<bool, T>,
        "This operator can be called for bool type only");
    return false;
  }

 private:
  T var_;
};

template <>
PARAOS_INLINE_TRIVIAL var_atomic<bool>::operator bool() const {
  const paraos::critical_section critical;  // RAII
  return var_;
}

using bool_atomic = var_atomic<bool>;
using BoolAtomic PARAOS_DEPRECATED("use paraos::bool_atomic") = bool_atomic;
template <typename T>
using VarAtomic PARAOS_DEPRECATED("use paraos::var_atomic") = var_atomic<T>;

}  // namespace paraos

#endif /* PARAOS_BOOL_ATOMIC_HPP */
