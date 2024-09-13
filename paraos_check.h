#ifndef IRTOS_CHECK_H
#define IRTOS_CHECK_H

#include <assert.h>

/* Define this macro to include build configuration header. This is an
 * alternative to the -D compiler flag.
 * Usage example with CMake:
 * "-DICORE_CHECK_CONFIG_HEADER="${CMAKE_CURRENT_SOURCE_DIR}/icore_check_config.h"
 * -----------------------------------------------------------------------------
 * or
 * -----------------------------------------------------------------------------
 * target_compile_definitions(
 *   interfaces_core INTERFACE
 *   -DICORE_CHECK_CONFIG_HEADER="${CMAKE_CURRENT_SOURCE_DIR}/icore_check_config.h")
 */
#ifdef PARAOS_CHECK_CONFIG_HEADER
#include PARAOS_CHECK_CONFIG_HEADER
#endif

#ifdef PARAOS_CHECK_LOOP_ENABLE
/* Если пользователь предоставил реализацию макроса LOOP */
#ifdef PARAOS_CHECK_LOOP_USER_IMP
#define PARAOS_CHECK_LOOP() PARAOS_CHECK_LOOP_USER_IMP()
#define PARAOS_CHECK_ASSERT(x) \
  if ((x) == 0) {              \
    PARAOS_CHECK_LOOP();       \
  }
#elif defined(__WIN32__) || defined(__WIN64__) || defined(__linux__) || \
    defined(__unix__)
/* Вызов макроса PARAOS_CHECK_LOOP() выполняет вызов assert() с ложным
 * условием */
#define PARAOS_CHECK_LOOP() assert(1 != 1)
#define PARAOS_CHECK_ASSERT(x) assert(x)
#else
/* В противном случае используется пустой макрос */
#define PARAOS_CHECK_LOOP()
#define PARAOS_CHECK_ASSERT(x)
#endif
#else
/**
 * @brief В соответствие с конфигурацией <PARAOS_CHECK_LOOP_ENABLE> и
 * <PARAOS_CHECK_LOOP_USER_IMP>, данный макрос является пустым
 */
#define PARAOS_CHECK_LOOP()

/**
 * @brief В соответствие с конфигурацией <PARAOS_CHECK_LOOP_ENABLE> и
 * <PARAOS_CHECK_LOOP_USER_IMP>, данный макрос является пустым
 */
#define PARAOS_CHECK_ASSERT(x)
#endif

#endif /* IRTOS_CHECK_H */
