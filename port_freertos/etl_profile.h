#ifndef ETL_PROFILE_H
#define ETL_PROFILE_H

#define ETL_THROW_EXCEPTIONS
#define ETL_VERBOSE_ERRORS

#if defined(_MSC_VER)
#include "etl/profiles/msvc_x86.h"
#elif defined(__GNUC__)
#include "etl/profiles/gcc_generic.h"
#endif

#define ETL_THROW_EXCEPTIONS
#define ETL_VERBOSE_ERRORS
#define ETL_CHECK_PUSH_POP

#define ETL_DEVELOPMENT_OS_WINDOWS

#define ETL_MESSAGE_TIMER_USE_ATOMIC_LOCK
#define ETL_CALLBACK_TIMER_DISABLE_INTERRUPTS
#define ETL_CALLBACK_TIMER_ENABLE_INTERRUPTS
#define ETL_TIMER_UPDATES_ENABLED false
#define ETL_CALLBACK_TIMER_USE_INTERRUPT_LOCK

#endif /* ETL_PROFILE_H */
