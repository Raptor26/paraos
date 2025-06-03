/// @file paraos_trace.hpp
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

#ifndef PARAOS_TRACE_HPP
#define PARAOS_TRACE_HPP

#ifdef paraosTRACE_ENABLE
#include <iostream>

#include "paraos_critical.hpp"

#define paraosTRACE_MESSAGE(__message__)          \
  {                                               \
    const paraos::CriticalSection macro_critical; \
    std::cout << __message__ << std::endl;        \
  }

#define paraosTRACE_MESSAGE_WITH_ACTOR_NAME(__message__, __object_name__) \
  {                                                                       \
    const paraos::CriticalSection macro_critical;                         \
    std::cout << "DM: '" << __object_name__ << "': " << __message__       \
              << std::endl;                                               \
  }

/// @brief Print command and the result.
/// @note https://en.cppreference.com/w/cpp/types/is_bounded_array.html
#define paraosOUT(...) std::cout << #__VA_ARGS__ << " : " << __VA_ARGS__ << '\n'

#else
#define paraosTRACE_MESSAGE(__message__)
#define paraosTRACE_MESSAGE_WITH_ACTOR_NAME(__message__, __object_name__)
#define paraosOUT(...)
#endif

#endif /* PARAOS_TRACE_HPP */
