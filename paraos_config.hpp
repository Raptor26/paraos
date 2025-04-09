/// @file paraos_config.hpp
///
/// Copyright 2008-2022 Emil Dotchevski and Reverge Studios, Inc.
/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef PARAOS_CONFIG_HPP
#define PARAOS_CONFIG_HPP

#if defined(PARAOS_STRICT_CONFIG) || defined(PARAOS_NO_WORKAROUNDS)
#define PARAOS_WORKAROUND(symbol, test) 0
#else
#define PARAOS_WORKAROUND(symbol, test) ((symbol) != 0 && ((symbol)test))
#endif

#define PARAOS_CLANG 0
#if defined(__clang__)
#undef PARAOS_CLANG
#define PARAOS_CLANG (__clang_major__ * 100 + __clang_minor__)
#endif

#if PARAOS_WORKAROUND(PARAOS_CLANG, < 304)
#define PARAOS_DEPRECATED(msg)
#elif defined(__GNUC__) || defined(__clang__)
#define PARAOS_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER) && _MSC_VER >= 1900
#define PARAOS_DEPRECATED(msg) [[deprecated(msg)]]
#else
#define PARAOS_DEPRECATED(msg)
#endif

#ifndef PARAOS_FORCEINLINE
#if defined(_MSC_VER)
#define PARAOS_FORCEINLINE __forceinline
#elif defined(__GNUC__) && __GNUC__ > 3
#define PARAOS_FORCEINLINE inline __attribute__((always_inline))
#else
#define PARAOS_FORCEINLINE inline
#endif
#endif

#ifndef PARAOS_INLINE
#define PARAOS_INLINE inline
#endif

#ifndef PARAOS_INLINE_TRIVIAL
#define PARAOS_INLINE_TRIVIAL PARAOS_FORCEINLINE
#endif

#ifndef PARAOS_INLINE_CRITICAL
#define PARAOS_INLINE_CRITICAL PARAOS_FORCEINLINE
#endif

#ifndef PARAOS_INLINE_OPERATIONS
#define PARAOS_INLINE_OPERATIONS PARAOS_INLINE
#endif

#ifndef PARAOS_INLINE_RECURSION
#define PARAOS_INLINE_RECURSION PARAOS_INLINE_OPERATIONS
#endif

#ifndef PARAOS_CONSTEXPR
#if __cplusplus >= 201703L
#define PARAOS_CONSTEXPR constexpr
#define PARAOS_MAYBE_UNUSED [[maybe_unused]]
#else
#define PARAOS_CONSTEXPR
#define PARAOS_MAYBE_UNUSED
#endif
#endif

#if PARAOS_USING_POLYMORPHIC_EXTRA
#define PARAOS_POLYMORPHIC_EXTRA virtual
#else
#define PARAOS_POLYMORPHIC_EXTRA
#endif

#endif /* PARAOS_CONFIG_HPP */
