#ifndef IRTOS_ATTR_H
#define IRTOS_ATTR_H

#define __ICORE_ATTR_UNUSED_VAR(X) (void)X

#if defined(__GNUC__)

#ifndef __ICORE_ATTR_PURE
#define __ICORE_ATTR_PURE __attribute__((pure))
#endif

/**
 *
 * The const function attribute specifies that a function examines only
 * its arguments, and has no effect except for the return value. That
 * is, the function does not read or modify any global memory. If a
 * function is known to operate only on its arguments then it can be
 * subject to common sub-expression elimination and loop optimizations.
 * This attribute is stricter than __attribute__((pure)) because
 * functions are not permitted to read global memory.
 *
 * // __attribute__((const)) functions do not read or modify any global
 * memory int my_double(int b) __attribute__((const)); int my_double(int
 * b) { return b*2;
 * }
 */
#ifndef __ICORE_ATTR_CONST
#define __ICORE_ATTR_CONST __attribute__((const))
#endif

/**
 * Where type is one of the following:
 * -IRQ.
 * -FIQ.
 * -SWI.
 * -ABORT.
 * -UNDEF.
 *
 * Usage:
 * The interrupt attribute affects the code generation of a function as
 * follows: If the function is AAPCS, the stack is realigned to 8 bytes
 * on entry. For processors that are not based on the M-profile,
 * preserves all processor registers, rather than only the registers
 * that the AAPCS requires to be preserved. Floating-point registers are
 * not preserved. For processors that are not based on the M-profile,
 * the function returns using an instruction that is architecturally
 * defined as a return from exception.
 *
 * Restrictions:
 * When using __attribute__((interrupt("type"))) functions:
 * No arguments or return values can be used with the functions.
 * The functions are incompatible with -frwpi.
 *
 * Note:
 * In ARMv6-M, ARMv7-M, and ARMv8-M, the architectural exception
 * handling mechanism preserves all processor registers, and a standard
 * function return can cause an exception return. Therefore, specifying
 * the interrupt attribute does not affect the behavior of the compiled
 * output. However, ARM recommends using the interrupt attribute on
 * exception handlers for clarity and easier software porting.
 *
 * Note:
 * For architectures that support A32 and T32 instructions, functions
 * specified with the interrupt attribute compile to A32 or T32 code
 * depending on whether the compile option specifies ARM or Thumb. For
 * Thumb only architectures, for example ARMv6-M, functions specified
 * with the interrupt attribute compile to T32 code. The interrupt
 * attribute is not available for A64 code.
 */
#ifndef __ICORE_ATTR_INTERRUPT_TYPE
#define __ICORE_ATTR_INTERRUPT_TYPE(type) __attribute__((interrupt("type")))
#endif

/**
 *
 * The deprecated variable attribute enables the declaration of a
 *deprecated variable without any warnings or errors being issued by the
 *compiler. However, any access to a deprecated variable creates a
 *warning but still compiles. The warning gives the location where the
 *variable is used and the location where it is defined. This helps you
 *to determine why a particular definition is deprecated.
 *
 * Example:
 * extern int deprecated_var __attribute__((deprecated));
 * void foo()
 * {
 *	deprecated_var=1;
 * }
 *
 */
#ifndef __ICORE_ATTR_DEPRECATED
#define __ICORE_ATTR_DEPRECATED __attribute__((deprecated))
#endif

/**
 * Generally, inlining into a function is limited. For a function marked
 * with this attribute, every call inside this function will be inlined,
 * if possible. Whether the function itself is considered for inlining
 * depends on its size and the current inlining parameters.
 */
#ifndef __ICORE_ATTR_FLATTEN
#define __ICORE_ATTR_FLATTEN __attribute__((flatten))
#endif

/**
 *
 * Functions defined with __attribute__((weak)) export their symbols
 * weakly. Functions declared with __attribute__((weak)) and then
 * defined without
 * __attribute__((weak)) behave as weak functions.
 *
 * Example:
 * extern int Function_Attributes_weak_0 (int b) __attribute__((weak));
 */
#if defined(_WIN32) || defined(_WIN64)
/* В случае использования компилятора GCC в составе mingw (т.е. под
 * windows), атрибут weak не поддерживается. Таким образом, макрос
 * __ICORE_ATTR_WEAK является пустым */
#ifndef __ICORE_ATTR_WEAK
#define __ICORE_ATTR_WEAK
#endif
#else
#ifndef __ICORE_ATTR_WEAK
#define __ICORE_ATTR_WEAK __attribute__((weak))
#endif
#endif

/**
 * In the following example, foo() calls y() through a weak reference:
 *
 * extern void y(void);
 * static void x(void) __attribute__((weakref("y")));
 * void foo (void)
 * {
 * 	...
 * 	x();
 * 	...
 * }
 *
 * Restrictions:
 * This attribute can only be used on functions with static linkage.
 */
#ifndef __ICORE_ATTR_WEAKREF
#define __ICORE_ATTR_WEAKREF(target) __attribute__((weakref("target")))
#endif

#ifndef __ICORE_ATTR_ALIAS
#define __ICORE_ATTR_ALIAS(target) __attribute__((alias("target")))
#endif

#ifndef __ICORE_ATTR_UNUSED
#define __ICORE_ATTR_UNUSED __attribute__((unused))
#endif

#ifndef __ICORE_ATTR_PACKED
#define __ICORE_ATTR_PACKED __attribute__((__packed__))
#endif

/* https://stackoverflow.com/questions/1537964/visual-c-equivalent-of-gccs-attribute-packed
 */
#ifndef __ICORE_PACK
#define __ICORE_PACK(__Declaration__) \
  __Declaration__ __attribute__((__packed__))
#endif

#ifndef __ICORE_ATTR_FNC_NO_OPTIMIZE
#define __ICORE_ATTR_FNC_NO_OPTIMIZE __attribute__((optimize("-O0")))
#endif

#elif defined(_MSC_VER)
#ifndef __ICORE_ATTR_WEAKREF
#define __ICORE_ATTR_WEAKREF(target) __attribute__((weakref("target")))
#endif

/**
 *
 * Functions defined with __attribute__((weak)) export their symbols weakly.
 * Functions declared with __attribute__((weak)) and then defined without
 * __attribute__((weak)) behave as weak functions.
 *
 * Example:
 * extern int Function_Attributes_weak_0 (int b) __attribute__((weak));
 */
#if defined(_WIN32) || defined(_WIN64)
/* В случае использования компилятора GCC в составе mingw (т.е. под
 * windows), атрибут weak не поддерживается. Таким образом, макрос
 * __ICORE_ATTR_WEAK является пустым */
#ifndef __ICORE_ATTR_WEAK
#define __ICORE_ATTR_WEAK
#endif
#else
#ifndef __ICORE_ATTR_WEAK
#define __ICORE_ATTR_WEAK __attribute__((weak))
#endif
#endif

#ifndef __ICORE_ATTR_ALIAS
#define __ICORE_ATTR_ALIAS(target) __attribute__((alias("target")))
#endif

#ifndef __ICORE_ATTR_UNUSED
#define __ICORE_ATTR_UNUSED __attribute__((unused))
#endif

/* https://stackoverflow.com/questions/1537964/visual-c-equivalent-of-gccs-attribute-packed
 */
#ifdef _MSC_VER
#define PACK(__Declaration__) \
  __pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))
#endif

#ifndef __ICORE_ATTR_FNC_NO_OPTIMIZE
#define __ICORE_ATTR_FNC_NO_OPTIMIZE __attribute__((optimize("-O0")))
#endif

#elif defined(_WIN64)
#define __ICORE_ATTR_CONST
#define __ICORE_ATTR_INTERRUPT_TYPE
#define __ICORE_ATTR_NORETURN
#define __ICORE_ATTR_DEPRECATED
#define __ICORE_ATTR_WEAK
#define __ICORE_ATTR_WEAKREF(target)
#define __ICORE_ATTR_ALIAS
#define __ICORE_ATTR_UNUSED
#define __ICORE_ATTR_PACKED  //__declspec(align(1))
#define __ICORE_ATTR_FNC_NO_OPTIMIZE
#else
#define __ICORE_ATTR_CONST
#define __ICORE_ATTR_INTERRUPT_TYPE
#define __ICORE_ATTR_NORETURN
#define __ICORE_ATTR_DEPRECATED
#define __ICORE_ATTR_WEAK
#define __ICORE_ATTR_WEAKREF(target) __attribute__((weakref("target")))
#define __ICORE_ATTR_ALIAS
#define __ICORE_ATTR_UNUSED

/* Данный макрос переопределяет уровень оптимизации функции на O0 */
#ifndef __ICORE_ATTR_FNC_NO_OPTIMIZE
#error "You must define this attribute"
#endif

#define __ICORE_ATTR_FNC_NO_OPTIMIZE
#if !defined(__ICORE_ATTR_PACKED)
#error \
    "You must define this attribute to disable optimization of fields for data structures"
#endif
#endif

#if defined(__GNUC__)

/* inline*/
#ifndef __icoreINLINE
#define __icoreINLINE inline
#endif

/* static inline */
#ifndef __icoreSTATIC_INLINE
#define __icoreSTATIC_INLINE static inline
#endif

/* always inline */
#ifndef __icoreALWAYS_INLINE
#define __icoreALWAYS_INLINE __attribute__((always_inline)) static inline
#endif

/* force inline */
#ifndef __icoreFORCE_INLINE
#define __icoreFORCE_INLINE static inline __attribute__((always_inline))
#endif

#else
#define __icoreINLINE static
#define __icoreSTATIC_INLINE static
#define __icoreALWAYS_INLINE static
#define __icoreFORCE_INLINE static
#endif

/// https://en.cppreference.com/w/cpp/language/attributes/noreturn
/// https://stackoverflow.com/questions/10538291/what-is-the-point-of-noreturn
#ifndef __ICORE_ATTR_NORETURN
#if defined(_MSC_VER)
#define __ICORE_ATTR_NORETURN __declspec(noreturn)
#elif defined(__GNUC__)
#define __ICORE_ATTR_NORETURN __attribute__((__noreturn__))
#elif defined(__has_attribute) && defined(__SUNPRO_CC) && (__SUNPRO_CC > 0x5130)
#if __has_attribute(noreturn)
#define __ICORE_ATTR_NORETURN [[noreturn]]
#endif
#elif defined(__has_cpp_attribute)
#if __has_cpp_attribute(noreturn)
#define __ICORE_ATTR_NORETURN [[noreturn]]
#endif
#else
#define __ICORE_ATTR_NORETURN
#endif
#endif

// Copyright 2008-2022 Emil Dotchevski and Reverge Studios, Inc.

#ifndef ICORE_CONFIG_HPP_INCLUDED
#define ICORE_CONFIG_HPP_INCLUDED

#if defined(ICORE_STRICT_CONFIG) || defined(ICORE_NO_WORKAROUNDS)
#define ICORE_WORKAROUND(symbol, test) 0
#else
#define ICORE_WORKAROUND(symbol, test) ((symbol) != 0 && ((symbol)test))
#endif

#define ICORE_CLANG 0
#if defined(__clang__)
#undef ICORE_CLANG
#define ICORE_CLANG (__clang_major__ * 100 + __clang_minor__)
#endif

#if ICORE_WORKAROUND(ICORE_CLANG, < 304)
#define ICORE_DEPRECATED(msg)
#elif defined(__GNUC__) || defined(__clang__)
#define ICORE_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER) && _MSC_VER >= 1900
#define ICORE_DEPRECATED(msg) [[deprecated(msg)]]
#else
#define ICORE_DEPRECATED(msg)
#endif

#ifndef ICORE_FORCEINLINE
#if defined(_MSC_VER)
#define ICORE_FORCEINLINE __forceinline
#elif defined(__GNUC__) && __GNUC__ > 3
#define ICORE_FORCEINLINE inline __attribute__((always_inline))
#else
#define ICORE_FORCEINLINE inline
#endif
#endif

#ifndef ICORE_INLINE
#define ICORE_INLINE inline
#endif

#ifndef ICORE_INLINE_TRIVIAL
#define ICORE_INLINE_TRIVIAL ICORE_FORCEINLINE
#endif

#ifndef ICORE_INLINE_CRITICAL
#define ICORE_INLINE_CRITICAL ICORE_FORCEINLINE
#endif

#ifndef ICORE_INLINE_OPERATIONS
#define ICORE_INLINE_OPERATIONS ICORE_INLINE
#endif

#ifndef ICORE_INLINE_RECURSION
#define ICORE_INLINE_RECURSION ICORE_INLINE_OPERATIONS
#endif

#ifndef ICORE_CONSTEXPR
#if __cplusplus >= 201703L
#define ICORE_CONSTEXPR constexpr
#else
#define ICORE_CONSTEXPR
#endif
#endif

#endif

#endif /* IRTOS_ATTR_H */
