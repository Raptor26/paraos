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
class VarAtomic final {
 public:
  explicit VarAtomic(T var) {
    const paraos::CriticalSection critical;  // RAII
    var_ = var;
  }

  ~VarAtomic() = default;

  // ---------------------------------------------------------------------------
  // Five Rule
  // ---------------------------------------------------------------------------

  PARAOS_INLINE_TRIVIAL VarAtomic(const VarAtomic& other) noexcept {
    const paraos::CriticalSection critical;  // RAII
    var_ = other.var_;
  }

  PARAOS_INLINE_TRIVIAL VarAtomic(VarAtomic&& other) noexcept {
    const paraos::CriticalSection critical;  // RAII
    var_ = std::move(other.var_);
  }

  PARAOS_INLINE_TRIVIAL auto operator=(const VarAtomic& other) noexcept
      -> VarAtomic& {
    if (this != &other) {
      const paraos::CriticalSection critical;  // RAII
      var_ = other.var_;
    }

    return *this;
  }

  PARAOS_INLINE_TRIVIAL auto operator=(VarAtomic&& other) noexcept
      -> VarAtomic& {
    const paraos::CriticalSection critical;  // RAII
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
PARAOS_INLINE_TRIVIAL VarAtomic<bool>::operator bool() const {
  const paraos::CriticalSection critical;  // RAII
  return var_;
}

using BoolAtomic = VarAtomic<bool>;

}  // namespace paraos

#endif /* PARAOS_BOOL_ATOMIC_HPP */
