/**
* @file endianness.hpp
*
* @author Mark Tikhonov <mtik.philosopher@gmail.com>
*
* @date 2025-08-05
*
* @copyright
* MIT License
* Copyright (c) 2025 Mark Tikhonov
* See the accompanying LICENSE file for details.
*/
#ifndef ESER_ENDIANNESS_HPP_
#define ESER_ENDIANNESS_HPP_
#pragma once

// First: attempt to include endian.h if the compiler supports __has_include
#if defined(__has_include)
    #if __has_include(<endian.h>)
        #include <endian.h>
    #endif
#endif

#if defined(__BYTE_ORDER)
// Now: try to detect if __BYTE_ORDER__ macros exist
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)

    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        constexpr bool host_is_little_endian = true;
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        constexpr bool host_is_little_endian = false;
    #else
        #error "Cannot determine endianness from __BYTE_ORDER__"
    #endif

#else

    // Fallback — require user to define explicitly
    #ifndef ETASK_FORCE_ENDIANNESS
        #error "Cannot auto-detect endianness. Please define ETASK_FORCE_ENDIANNESS"
    #else
        constexpr bool host_is_little_endian = ETASK_FORCE_ENDIANNESS;
    #endif

#endif
#endif

#endif // ESER_ENDIANNESS_HPP_