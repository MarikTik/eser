/**
* @file endianness.hpp
*
* @brief Public byte-order surface: the `endianness` enum and the `is_endianness_neutral` trait.
*
* @ingroup eser_utils
*
* These are the only endianness names users touch: the `endianness` enum (passed to
* `serialize<endianness::big>(...)` / `deserialize<endianness::big>(...)`) and the
* `is_endianness_neutral` customization point (specialize it for your own byte-only value types so
* they may cross a non-native wire). The host-detection and byte-reversal machinery lives in
* `eser/internal/endianness.hpp` and is not part of the public API.
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
* - 2025-08-05
* -     Initial creation (host-detection only).
* - 2026-06-24
* -     Split: the public `endianness` enum and `is_endianness_neutral` trait stay here; the
*       host detection and `reverse_bytes` / `apply_wire_endianness` machinery moved to
*       `eser/internal/endianness.hpp`.
*/
#ifndef ESER_UTILS_ENDIANNESS_HPP_
#define ESER_UTILS_ENDIANNESS_HPP_
#include <type_traits>

namespace eser::utils{
    /**
    * @enum endianness
    * @brief Byte order of a multi-byte scalar on the wire or on the host.
    */
    enum class endianness{
        little, ///< Least-significant byte first.
        big     ///< Most-significant byte first (a.k.a. network byte order).
    };

    /**
    * @struct is_endianness_neutral
    * @brief Marks a type whose byte order is irrelevant (it never needs byte-swapping).
    *
    * A type is endianness-neutral when its meaningful units are single bytes, so it round-trips
    * identically under any wire byte order — e.g. a fixed-capacity character string. Such types are
    * passed through unchanged when the wire order differs from the host, instead of being rejected
    * like a general multi-byte struct.
    *
    * The primary template is `false`; **specialize it (to `std::true_type`) for your own byte-only
    * value types** to allow them across a non-native wire. This is a supported customization point.
    *
    * @tparam T The type to inspect.
    *
    * @see is_endianness_neutral_v
    */
    template <typename T>
    struct is_endianness_neutral : std::false_type {};

    /**
    * @var is_endianness_neutral_v
    * @brief Convenience variable template for `is_endianness_neutral<T>::value`.
    *
    * @tparam T The type to inspect.
    *
    * @see is_endianness_neutral
    */
    template <typename T>
    inline constexpr bool is_endianness_neutral_v = is_endianness_neutral<T>::value;
} // namespace eser::utils

#endif // ESER_UTILS_ENDIANNESS_HPP_
