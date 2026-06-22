/// @file paraos_trace.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
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
