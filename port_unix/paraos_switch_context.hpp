/// @file paraos_switch_context.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
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

#ifndef PARAOS_SWITCH_CONTEXT_HPP
#define PARAOS_SWITCH_CONTEXT_HPP

#include "paraos_attr.h"
#include "paraos_isr.hpp"

namespace paraos {

/// @brief Automaticaly switch thread context if need. Use WritePrimitiveState()
/// for check is need switch thread context.
///
/// @note paraos not support execute from isr in unix. In this case, unix
/// port use mock for SwitchContext().
///
/// @tparam IS_ISR: Set true if use SwitchContext in ISR
///
template <bool IS_ISR = true>
class SwitchContext final {
 public:
  SwitchContext() = default;

  void WritePrimitiveState(const ISRbool &primitive_state) {
    PARAOS_ATTR_UNUSED_VAR(primitive_state);
  }

  ~SwitchContext() {}
};

}  // namespace paraos

#endif /* PARAOS_SWITCH_CONTEXT_HPP */
