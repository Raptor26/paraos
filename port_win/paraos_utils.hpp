/// @file paraos_utils.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef UTILS_HPP
#define UTILS_HPP

// NOLINTBEGIN(llvm-include-order)
// clang-format off
// winsock2.h must include before windows.h
#include <winsock2.h>
#include <windows.h>
// clang-format on
// NOLINTEND(llvm-include-order)

#include <minwindef.h>

#include <cassert>
#include <cstddef>

#include "paraos_config.hpp"

namespace paraos {

using delay_type = DWORD;

constexpr delay_type max_delay{INFINITE};
static_assert(sizeof(max_delay) >= sizeof(DWORD));

constexpr auto get_stack_minimum_size_in_bytes() -> std::size_t {
  constexpr size_t stack_multiplier{1024};
  return stack_multiplier * sizeof(size_t);
}

PARAOS_DEPRECATED("use get_stack_minimum_size_in_bytes()")
constexpr auto GetStackMinimumSizeInBytes() -> std::size_t {
  return get_stack_minimum_size_in_bytes();
}

}  // namespace paraos

#endif /* UTILS_HPP */
