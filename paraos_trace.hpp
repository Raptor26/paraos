#ifndef PARAOS_TRACE_HPP
#define PARAOS_TRACE_HPP

#ifdef paraosTRACE_ENABLE
#include <iostream>
#define paraosTRACE_MESSAGE(message) std::cout << message << std::endl
#else
#define paraosTRACE_MESSAGE(message)
#endif

#endif /* PARAOS_TRACE_HPP */
