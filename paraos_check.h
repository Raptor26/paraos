/// @file paraos_check.h
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_CHECK_H
#define PARAOS_CHECK_H

#include <assert.h>

/* Define this macro to include build configuration header. This is an
 * alternative to the -D compiler flag.
 * Usage example with CMake:
 * "-DPARAOS_CHECK_CONFIG_HEADER="${CMAKE_CURRENT_SOURCE_DIR}/paraos_check_config.hpp"
 * -----------------------------------------------------------------------------
 * or (example)
 * -----------------------------------------------------------------------------
 * target_compile_definitions(
 *   paraos_setup INTERFACE
 *   -DPARAOS_CHECK_CONFIG_HEADER="${CMAKE_CURRENT_SOURCE_DIR}/paraos_check_config.hpp")
 */
#ifdef PARAOS_CHECK_CONFIG_HEADER
#include PARAOS_CHECK_CONFIG_HEADER
#endif

#ifdef PARAOS_CHECK_LOOP_ENABLE

// If build system is windows or linux, we can provide definitions for
// PARAOS_CHECK_LOOP() and PARAOS_CHECK_ASSERT()
#if defined(__WIN32__) || defined(__WIN64__) || defined(__linux__) || \
    defined(__unix__) || defined(__APPLE__)

#ifndef PARAOS_CHECK_LOOP
#define PARAOS_CHECK_LOOP() assert(false)
#endif

#ifndef PARAOS_CHECK_ASSERT
#define PARAOS_CHECK_ASSERT(x) assert(x)
#endif

#elif !defined(PARAOS_CHECK_LOOP)
#error \
    "User code must provide PARAOS_CHECK_LOOP() macros for host platform which loop forever"
#else
// In other case, user code must provide PARAOS_CHECK_LOOP() definition and
// after that we can provide PARAOS_CHECK_ASSERT() definition
#define PARAOS_CHECK_ASSERT(x) \
  if ((x) == 0) {              \
    PARAOS_CHECK_LOOP();       \
  }
#endif
#else

#ifndef PARAOS_CHECK_LOOP
#define PARAOS_CHECK_LOOP()
#endif

#ifndef PARAOS_CHECK_ASSERT
#define PARAOS_CHECK_ASSERT(x)
#endif

#endif /* #ifdef PARAOS_CHECK_LOOP_ENABLE */

#endif /* PARAOS_CHECK_H */
