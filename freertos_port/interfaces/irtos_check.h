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
#ifdef ICORE_CHECK_CONFIG_HEADER
#include ICORE_CHECK_CONFIG_HEADER
#endif

#ifdef icore_checkLOOP_ENABLE
/* Если пользователь предоставил реализацию макроса LOOP */
#ifdef __icore_checkLOOP_USER_IMP
#define __icore_checkLOOP() __icore_checkLOOP_USER_IMP()
#define __icore_checkASSERT(x) \
  if ((x) == 0) {              \
    __icore_checkLOOP();       \
  }
#elif defined(__WIN32__) || defined(__WIN64__)
/* Вызов макроса __icore_checkLOOP() выполняет вызов assert() с ложным
 * условием */
#define __icore_checkLOOP() assert(1 != 1)
#define __icore_checkASSERT(x) \
  if ((x) == 0) {              \
    __icore_checkLOOP();       \
  }
#else
/* В противном случае используется пустой макрос */
#define __icore_checkLOOP()
#define __icore_checkASSERT(x)
#endif
#else
/**
 * @brief В соответствие с конфигурацией <icore_checkLOOP_ENABLE> и
 * <__icore_checkLOOP_USER_IMP>, данный макрос является пустым
 */
#define __icore_checkLOOP()

/**
 * @brief В соответствие с конфигурацией <icore_checkLOOP_ENABLE> и
 * <__icore_checkLOOP_USER_IMP>, данный макрос является пустым
 */
#define __icore_checkASSERT(x)
#endif

#endif /* IRTOS_CHECK_H */
