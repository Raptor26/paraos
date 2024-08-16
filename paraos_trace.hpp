#ifndef PARAOS_TRACE_HPP
#define PARAOS_TRACE_HPP

#ifdef paraosTRACE_ENABLE
#include <iostream>

#include "paraos_critical.hpp"

#define paraosTRACE_MESSAGE(__message__)    \
  {                                         \
    const paraos::CriticalSection critical; \
    std::cout << __message__ << std::endl;  \
  }

#else
#define paraosTRACE_MESSAGE(message)
#endif

#endif /* PARAOS_TRACE_HPP */
