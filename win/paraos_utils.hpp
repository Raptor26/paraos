#ifndef UTILS_HPP
#define UTILS_HPP

#include <windows.h>

#include <cassert>
#include <cstddef>

namespace paraos {

constexpr std::size_t max_delay{INFINITE};
static_assert(sizeof(max_delay) >= sizeof(DWORD));

}  // namespace paraos

#endif /* UTILS_HPP */
