#ifndef PARAOS_FREERTOS_HOOKS_HPP
#define PARAOS_FREERTOS_HOOKS_HPP

#include <type_traits>

namespace paraos {
using FreeRTOSIdleFncPtr = void (*)();

inline FreeRTOSIdleFncPtr freertos_idle_fnc_ptr{nullptr};

// freertos_idle_fnc_ptr = ExitAfterTestCompelte();
}  // namespace paraos

#endif /* PARAOS_FREERTOS_HOOKS_HPP */
