/**
* @file constexpr_assert.hpp
*
* @brief `assert`, kept usable inside a `constexpr` function on every toolchain.
*
* @ingroup eser_internal
*
* Asserting inside a `constexpr` function is ordinary, standard C++: `assert`
* expands to a conditional expression, which vanishes during constant evaluation
* and traps at runtime. That is how it behaves under glibc, libc++, and MSVC.
*
* Some embedded toolchains break it. The Xtensa (ESP8266) newlib defines:
*
* @code
* #define assert(__e) ((__e) ? (void)0 : __assert_func (PSTR(__FILE__), __LINE__, \
*                                                       __ASSERT_FUNC, PSTR(#__e)))
* @endcode
*
* and `PSTR(s)` is a statement expression declaring a **static** array, so the
* message lives in flash rather than RAM. A `static` variable inside a
* `constexpr` function is ill-formed in C++17, so any `constexpr` function
* containing an `assert` fails to compile there:
*
* @code
* error: '__pstr__' declared 'static' in 'constexpr' function
*     assert(data != nullptr && "Data pointer is null");
* @endcode
*
* The AVR core's `assert` has the same shape. Neither is a mistake in the
* asserting code, and neither is something the caller can fix.
*
* @ref ESER_CONSTEXPR_ASSERT keeps the real `assert` - message, file, line -
* wherever it works, and drops it only on the toolchains where it cannot be
* written at all. The check is worth having on a device; it is worth having on
* every device that can compile it.
*
* @author Mark Tikhonov <mtik.philosopher@gmail.com>
*
* @date 2026-08-27
*
* @copyright
* MIT License
* SPDX-License-Identifier: MIT
*/
#ifndef ESER_INTERNAL_CONSTEXPR_ASSERT_HPP_
#define ESER_INTERNAL_CONSTEXPR_ASSERT_HPP_
#include <cassert>

/**
* @def ESER_ASSERT_IS_CONSTEXPR_SAFE
*
* @brief Whether this toolchain's `assert` may appear in a `constexpr` function.
*
* False when `PSTR` is defined, which is how the Arduino-derived toolchains
* (Xtensa/ESP8266, AVR) place an assertion's strings in flash - by declaring a
* `static` array, which C++17 forbids inside a `constexpr` function. `PSTR` is
* the discriminator rather than a target macro because it is the *cause*: a
* toolchain that stops routing `assert` through it becomes safe automatically,
* and one that starts is caught without this header knowing its name.
*
* Define it yourself to override, if a platform is misdetected either way.
*/
#ifndef ESER_ASSERT_IS_CONSTEXPR_SAFE
#  ifdef PSTR
#    define ESER_ASSERT_IS_CONSTEXPR_SAFE 0
#  else
#    define ESER_ASSERT_IS_CONSTEXPR_SAFE 1
#  endif
#endif

/**
* @def ESER_CONSTEXPR_ASSERT
*
* @brief `assert(condition)`, or nothing where `assert` cannot be written.
*
* Use in place of `assert` inside any `constexpr` function. On a conforming
* toolchain this *is* `assert`, with its message and source location intact; on
* one whose macro is unusable in a constant expression it expands to nothing,
* and the condition remains a documented precondition.
*
* @param condition The predicate to check. Write it exactly as for `assert`,
*                  including the `&& "why"` idiom.
*/
#if ESER_ASSERT_IS_CONSTEXPR_SAFE
#  define ESER_CONSTEXPR_ASSERT(condition) assert(condition)
#else
#  define ESER_CONSTEXPR_ASSERT(condition) ((void)0)
#endif

#endif // ESER_INTERNAL_CONSTEXPR_ASSERT_HPP_
