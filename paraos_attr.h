/// @file paraos_attr.h
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
#ifndef PARAOS_ATTR_H
#define PARAOS_ATTR_H

#define PARAOS_ATTR_UNUSED_VAR(X) (void)X

#if defined(__GNUC__)

#ifndef PARAOS_ATTR_PURE
#define PARAOS_ATTR_PURE __attribute__((pure))
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
#ifndef PARAOS_ATTR_CONST
#define PARAOS_ATTR_CONST __attribute__((const))
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
#ifndef PARAOS_ATTR_INTERRUPT_TYPE
#define PARAOS_ATTR_INTERRUPT_TYPE(type) __attribute__((interrupt("type")))
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
#ifndef PARAOS_ATTR_DEPRECATED
#define PARAOS_ATTR_DEPRECATED __attribute__((deprecated))
#endif

/**
 * Generally, inlining into a function is limited. For a function marked
 * with this attribute, every call inside this function will be inlined,
 * if possible. Whether the function itself is considered for inlining
 * depends on its size and the current inlining parameters.
 */
#ifndef PARAOS_ATTR_FLATTEN
#define PARAOS_ATTR_FLATTEN __attribute__((flatten))
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
#ifndef PARAOS_ATTR_WEAKREF
#define PARAOS_ATTR_WEAKREF(target) __attribute__((weakref("target")))
#endif

#ifndef PARAOS_ATTR_ALIAS
#define PARAOS_ATTR_ALIAS(target) __attribute__((alias("target")))
#endif

#ifndef PARAOS_ATTR_UNUSED
#define PARAOS_ATTR_UNUSED __attribute__((unused))
#endif

#ifndef PARAOS_ATTR_PACKED
#define PARAOS_ATTR_PACKED __attribute__((__packed__))
#endif

#ifndef PARAOS_ATTR_FNC_NO_OPTIMIZE
#define PARAOS_ATTR_FNC_NO_OPTIMIZE __attribute__((optimize("-O0")))
#endif

#elif defined(_MSC_VER)
#ifndef PARAOS_ATTR_WEAKREF
#define PARAOS_ATTR_WEAKREF(target) __attribute__((weakref("target")))
#endif

#ifndef PARAOS_ATTR_ALIAS
#define PARAOS_ATTR_ALIAS(target) __attribute__((alias("target")))
#endif

#ifndef PARAOS_ATTR_UNUSED
#define PARAOS_ATTR_UNUSED __attribute__((unused))
#endif

#ifndef PARAOS_ATTR_FNC_NO_OPTIMIZE
#define PARAOS_ATTR_FNC_NO_OPTIMIZE __attribute__((optimize("-O0")))
#endif

#elif defined(_MSC_VER)
#define PARAOS_ATTR_CONST
#define PARAOS_ATTR_INTERRUPT_TYPE
#define PARAOS_ATTR_NORETURN
#define PARAOS_ATTR_DEPRECATED
#define PARAOS_ATTR_WEAKREF(target)
#define PARAOS_ATTR_ALIAS
#define PARAOS_ATTR_UNUSED
#define PARAOS_ATTR_PACKED  //__declspec(align(1))
#define PARAOS_ATTR_FNC_NO_OPTIMIZE
#else
#define PARAOS_ATTR_CONST
#define PARAOS_ATTR_INTERRUPT_TYPE
#define PARAOS_ATTR_NORETURN
#define PARAOS_ATTR_DEPRECATED
#define PARAOS_ATTR_WEAKREF(target) __attribute__((weakref("target")))
#define PARAOS_ATTR_ALIAS
#define PARAOS_ATTR_UNUSED

/* Данный макрос переопределяет уровень оптимизации функции на O0 */
#ifndef PARAOS_ATTR_FNC_NO_OPTIMIZE
#error "You must define this attribute"
#endif

#define PARAOS_ATTR_FNC_NO_OPTIMIZE
#if !defined(PARAOS_ATTR_PACKED)
#error \
    "You must define this attribute to disable optimization of fields for data structures"
#endif
#endif

/// https://en.cppreference.com/w/cpp/language/attributes/noreturn
/// https://stackoverflow.com/questions/10538291/what-is-the-point-of-noreturn
#ifndef PARAOS_ATTR_NORETURN
#if defined(_MSC_VER)
#define PARAOS_ATTR_NORETURN __declspec(noreturn)
#elif defined(__GNUC__)
#define PARAOS_ATTR_NORETURN __attribute__((__noreturn__))
#elif defined(__has_attribute) && defined(__SUNPRO_CC) && (__SUNPRO_CC > 0x5130)
#if __has_attribute(noreturn)
#define PARAOS_ATTR_NORETURN [[noreturn]]
#endif
#elif defined(__has_cpp_attribute)
#if __has_cpp_attribute(noreturn)
#define PARAOS_ATTR_NORETURN [[noreturn]]
#endif
#else
#define PARAOS_ATTR_NORETURN
#endif
#endif

// Weak attribute definition ---------------------------------------------------
//
// Functions defined with __attribute__((weak)) export their symbols weakly.
// Functions declared with __attribute__((weak)) and then defined without
// __attribute__((weak)) behave as weak functions.
//
// Example:
// extern int Function_Attributes_weak_0 (int b) __attribute__((weak));
#if defined(_MSC_VER) || defined(_WIN32) || defined(_WIN64)
// MINGW compiler, as Microsoft Visual Studio not support weak attribute.
#define PARAOS_ATTR_WEAK
#elif defined(__GNUC__) || defined(__clang__)
#define PARAOS_ATTR_WEAK __attribute__((weak))
#endif

// Struct packed attribute definition ------------------------------------------
#if defined(__GNUC__) || defined(__clang__)
#define PARAOS_PACK(__Declaration__) __Declaration__ __attribute__((__packed__))
#elif defined(_MSC_VER)
#define PARAOS_PACK(__Declaration__) \
  __pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))
#else
#error "Need definition PARAOS_PACK for used compiler"
#endif

#define PARAOS_PACK_STRUCT(__Declaration__) \
  struct __Declaration__ __attribute__((__packed__))

/// ----------------------------------------------------------------------------
/// The macros below are useful for preventing optimization of structures and
/// classes with template parameters.
///
/// <pre>
/// {@code
/// PARAOS_NO_PADDING_NO_OPTIMIZE_BEGIN
/// template <typename T>
/// struct MyStruct {
///   char a;
///   int b;
///   T t;
/// };
/// PARAOS_NO_PADDING_NO_OPTIMIZE_END
/// }
/// </pre>
#if defined(__GNUC__) || defined(__clang__)

#define PARAOS_NO_PADDING_NO_OPTIMIZE_BEGIN _Pragma("pack(push, 1)")
#define PARAOS_NO_PADDING_NO_OPTIMIZE_END _Pragma("pack(pop)")
#elif defined(_MSC_VER)
#define PARAOS_NO_PADDING_NO_OPTIMIZE_BEGIN __pragma(pack(push, 1))
#define PARAOS_NO_PADDING_NO_OPTIMIZE_END __pragma(pack(pop))
#else
#error "Compiler does not support packing directives"
#define PARAOS_NO_PADDING_NO_OPTIMIZE_BEGIN
#define PARAOS_NO_PADDING_NO_OPTIMIZE_END
#endif

#endif /* PARAOS_ATTR_H */
