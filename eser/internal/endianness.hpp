/**
* @file endianness.hpp
*
* @brief Internal byte-order machinery: host detection and byte-order conversion.
*
* @ingroup eser_internal
*
* @warning Implementation detail. Do not include directly or depend on `eser::internal`; it is not
*          part of the public API and may change between releases. The public byte-order surface
*          (the `endianness` enum and `is_endianness_neutral`) lives in `eser/utils/endianness.hpp`.
*
* @note C++17 has no `std::endian`, so the host order is detected from `__BYTE_ORDER__`
*       (GCC/Clang) with an MSVC fallback (its targets are little-endian) and a final
*       `ESER_FORCE_ENDIANNESS_LITTLE` / `ESER_FORCE_ENDIANNESS_BIG` escape hatch.
*
* @author Mark Tikhonov <mtik.philosopher@gmail.com>
*
* @date 2026-06-24
*
* @copyright
* MIT License
* SPDX-License-Identifier: MIT
*
* @par Changelog
* - 2026-06-24
* -     Initial creation (split from eser/utils/endianness.hpp): host detection plus the
*       `reverse_bytes` / `apply_wire_endianness` conversion helpers.
*/
#ifndef ESER_INTERNAL_ENDIANNESS_HPP_
#define ESER_INTERNAL_ENDIANNESS_HPP_
#include <cstddef>
#include <type_traits>
#include "traits.hpp"
#include "../utils/endianness.hpp"

namespace eser::internal{
    using eser::utils::endianness;
    using eser::utils::is_endianness_neutral_v;

    /**
    * @var host_endianness
    * @brief The byte order of the machine this translation unit is compiled for.
    *
    * Detected from `__BYTE_ORDER__` when available, with an MSVC fallback and a
    * `ESER_FORCE_ENDIANNESS_LITTLE` / `ESER_FORCE_ENDIANNESS_BIG` override of last resort.
    */
    #if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
        #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            inline constexpr endianness host_endianness = endianness::little;
        #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            inline constexpr endianness host_endianness = endianness::big;
        #else
            #error "[eser] unsupported __BYTE_ORDER__ (PDP/mixed endianness is not supported)"
        #endif
    #elif defined(_MSC_VER)
        // MSVC only targets little-endian architectures (x86, x64, ARM/ARM64 in LE mode).
        inline constexpr endianness host_endianness = endianness::little;
    #elif defined(ESER_FORCE_ENDIANNESS_LITTLE)
        inline constexpr endianness host_endianness = endianness::little;
    #elif defined(ESER_FORCE_ENDIANNESS_BIG)
        inline constexpr endianness host_endianness = endianness::big;
    #else
        #error "[eser] cannot detect host endianness; define ESER_FORCE_ENDIANNESS_LITTLE or ESER_FORCE_ENDIANNESS_BIG"
    #endif

    /**
    * @brief Reverse the object representation of `value` in place.
    *
    * Swaps the `sizeof(T)` bytes of `value` end-for-end, converting between little- and
    * big-endian. A one-byte type is a no-op.
    *
    * @tparam T A trivially-copyable type (scalar, enum, or floating-point).
    * @param value The value whose bytes are reversed in place.
    *
    * @note Operates on the raw bytes, so it is correct for integers, enums, and IEEE-754
    *       floating-point (whose byte order tracks the integer byte order on supported targets).
    */
    template<typename T>
    inline void reverse_bytes(T& value) noexcept
    {
        static_assert(std::is_trivially_copyable_v<T>, "reverse_bytes requires a trivially-copyable type");
        auto* bytes = reinterpret_cast<unsigned char*>(&value);
        for (std::size_t i = 0, j = sizeof(T) - 1; i < j; ++i, --j)
        {
            const unsigned char tmp = bytes[i];
            bytes[i] = bytes[j];
            bytes[j] = tmp;
        }
    }

    /**
    * @brief Convert `value` between the wire byte order and the host byte order.
    *
    * A no-op when `Wire == host_endianness`. Otherwise:
    * - endianness-neutral types (`is_endianness_neutral`, e.g. byte-string fields) pass through;
    * - scalars, enums and floats are byte-reversed (@ref reverse_bytes);
    * - `std::array` elements are converted individually;
    * - other trivially-copyable structs are rejected (`static_assert`) — raw bytes carry no type
    *   information to swap, so a non-native wire order would corrupt their members.
    *
    * @tparam Wire The byte order of the serialized stream.
    * @tparam T    The (trivially-copyable) field type being converted.
    * @param value The field to convert in place.
    */
    template<endianness Wire, typename T>
    inline void apply_wire_endianness(T& value) noexcept
    {
        if constexpr (Wire != host_endianness)
        {
            if constexpr (is_endianness_neutral_v<T>)
            {
                // byte-only type: order is irrelevant, leave the bytes untouched
            }
            else if constexpr (is_std_array_v<T>)
            {
                for (auto& element : value) apply_wire_endianness<Wire>(element);
            }
            else
            {
                static_assert(not std::is_class_v<T>,
                    "[eser] trivially-copyable structs are stored as raw bytes and cannot be "
                    "byte-swapped for a non-native wire endianness; use a native-endianness codec, "
                    "split the struct into scalar fields, or specialize is_endianness_neutral if the "
                    "type is byte-only");
                reverse_bytes(value);
            }
        }
        (void)value; // silence unused warning when Wire == host_endianness
    }
} // namespace eser::internal

#endif // ESER_INTERNAL_ENDIANNESS_HPP_
