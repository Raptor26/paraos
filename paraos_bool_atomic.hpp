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

#include "paraos_config.hpp"
#include "paraos_critical.hpp"

namespace paraos {

/// @brief Class for atomic modify bool variable.
class BoolAtomic final {
 public:
  /// @brief Ctor.
  /// @param
  BoolAtomic(bool new_status) noexcept : bool_value_{new_status} {}

  /// @brief Default Ctor
  BoolAtomic() noexcept : BoolAtomic{false} {}

  // ---------------------------------------------------------------------------
  // Five Rule
  // ---------------------------------------------------------------------------

  PARAOS_INLINE_TRIVIAL BoolAtomic(const BoolAtomic& other) noexcept {
    const paraos::CriticalSection critical;  // RAII
    bool_value_ = other.bool_value_;
  }

  PARAOS_INLINE_TRIVIAL BoolAtomic(BoolAtomic&& other) noexcept {
    const paraos::CriticalSection critical;  // RAII
    bool_value_ = other.bool_value_;
  }

  PARAOS_INLINE_TRIVIAL BoolAtomic& operator=(
      const BoolAtomic& other) noexcept {
    if (this != &other) {
      const paraos::CriticalSection critical;  // RAII
      bool_value_ = other.bool_value_;
    }

    return *this;
  }

  PARAOS_INLINE_TRIVIAL BoolAtomic& operator=(BoolAtomic&& other) noexcept {
    const paraos::CriticalSection critical;  // RAII
    bool_value_ = other.bool_value_;

    return *this;
  }
  // ---------------------------------------------------------------------------

  PARAOS_INLINE_TRIVIAL operator bool() const noexcept { return Islocked(); }

 private:
  /// @brief Safe thread getter status.
  /// @return true or false.
  PARAOS_INLINE_TRIVIAL bool Islocked() const noexcept {
    const paraos::CriticalSection critical;  // RAII
    return bool_value_;
  }

 private:
  bool bool_value_;
};

}  // namespace paraos

#endif /* PARAOS_BOOL_ATOMIC_HPP */
