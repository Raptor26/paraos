/// @file paraos_bool_atomic.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright Copyright (c) 2024 Stilsoft
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

#ifndef PARAOS_BOOL_ATOMIC_HPP
#define PARAOS_BOOL_ATOMIC_HPP

#include <cassert>
#include <type_traits>

#include "paraos_config.hpp"
#include "paraos_critical.hpp"

namespace paraos {

template <typename T>
class VarAtomic final {
 public:
  VarAtomic(T var) {
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

  PARAOS_INLINE_TRIVIAL VarAtomic& operator=(const VarAtomic& other) noexcept {
    if (this != &other) {
      const paraos::CriticalSection critical;  // RAII
      var_ = other.var_;
    }

    return *this;
  }

  PARAOS_INLINE_TRIVIAL VarAtomic& operator=(VarAtomic&& other) noexcept {
    const paraos::CriticalSection critical;  // RAII
    var_ = std::move(other.var_);

    return *this;
  }
  // ---------------------------------------------------------------------------

  operator bool() const {
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
