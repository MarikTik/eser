/**
* @file byte.hpp
*
* @brief Toolchain requirements guard for `std::byte` (and C++17).
*
* @ingroup eser_utils
*
* `eser` represents raw memory as `std::byte` and is written against C++17. This header is the
* single place those two requirements are checked, so a violation fails with one clear message
* instead of a wall of template errors deep in the codec.
*
* It does **not** provide a fallback `std::byte`: a prior version injected one into namespace
* `std`, which is undefined behavior ([namespace.std]) and incomplete anyway (the real `std::byte`
* carries `std::to_integer` and bitwise operators). In practice the fallback also guarded an
* impossible case — any standard library providing `std::optional`, `std::string_view`,
* `std::tuple`, etc. (all of which `eser` requires) also provides `std::byte`. So instead of
* patching, this header asserts the requirement and stops.
*
* @author Mark Tikhonov <mtik.philosopher@gmail.com>
*
* @date 2025-07-02
*
* @copyright
* MIT License
* SPDX-License-Identifier: MIT
*
* @par Changelog
* - 2025-07-02
* -     Initial creation.
* - 2025-08-05
* -     License changed from CC BY-ND 4.0 to MIT. Library renamed from `ser` to `eser`.
* - 2026-06-24
* -     Removed the undefined-behavior `std::byte` fallback; reduced to a requirements guard
*       (C++17 + `std::byte`), with an MSVC-safe language-version check.
*/
#ifndef ESER_UTILS_BYTE_HPP_
#define ESER_UTILS_BYTE_HPP_

// MSVC reports C++17 via _MSVC_LANG unless /Zc:__cplusplus is set; prefer it when present.
#if (defined(_MSVC_LANG) ? _MSVC_LANG : __cplusplus) < 201703L
    #error "[eser] C++17 or newer is required."
#endif

#include <cstddef>

#if !defined(__cpp_lib_byte)
    #error "[eser] std::byte is required but is not provided by <cstddef>; use a conforming C++17 standard library."
#endif

#endif // ESER_UTILS_BYTE_HPP_
