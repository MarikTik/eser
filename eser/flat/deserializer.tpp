/**
* @file deserializer.tpp
*
* @brief Definition of functionality in deserializer.hpp
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
*       License changed from CC BY-ND 4.0 to MIT.
*       Library renamed from `ser` to `eser`
* - 2026-06-24
*       `to()` overloads return `std::optional<...>`: `std::nullopt` on insufficient
*       bytes, otherwise the engaged value. Removed the silent zero-fill; the public
*       `to()` now guarantees the bytes exist before any read.
*
*       `to()` became single-parameter; multi-field reads name a `std::tuple`. The
*       per-array `deserialize_impl` was removed — `std::array` is trivially copyable and
*       goes through the single memcpy reader like any other leaf value.
*/
#ifndef ESER_FLAT_DESERIALIZER_TPP_
#define ESER_FLAT_DESERIALIZER_TPP_
#include "deserializer.hpp"
#include "../utils/endianness.hpp"
#include <cassert>
#include <utility>
#include <array>
#include <cstring>
#include <limits>
namespace eser::flat{
    template<endianness Wire>
    template<typename Tuple, std::enable_if_t<utils::is_tuple_v<Tuple>, bool>>
    inline std::optional<Tuple> deserializer<Wire>::to() noexcept
    {
        return to_impl(utils::type_identity<Tuple>{});
    }

    template<endianness Wire>
    template<typename T, std::enable_if_t<
        std::is_trivially_copyable_v<T> &&
        !std::is_array_v<T> &&
        !utils::is_tuple_v<T>, bool>
    >
    inline std::optional<T> deserializer<Wire>::to() noexcept
    {
        if (_length < sizeof(T)) return std::nullopt;
        return deserialize_impl<T>();
    }

    template<endianness Wire>
    template<typename... Es>
    inline std::optional<std::tuple<Es...>> deserializer<Wire>::to_impl(utils::type_identity<std::tuple<Es...>>) noexcept
    {
        static_assert(sizeof...(Es) > 0, "Cannot deserialize an empty std::tuple<>; name at least one field");
        constexpr std::size_t bytes_required = (sizeof(Es) + ...);
        if (_length < bytes_required) return std::nullopt;
        // Braced init guarantees left-to-right evaluation, so each deserialize_impl
        // advances the cursor in field order; a parenthesised tuple ctor would not.
        return std::tuple<Es...>{ deserialize_impl<Es>()... };
    }

    template<endianness Wire>
    template<typename T>
    inline T deserializer<Wire>::deserialize_impl() noexcept
    {
        static_assert(std::is_trivially_copyable_v<T>, "deserialize_impl requires a trivially-copyable type");
        if constexpr (std::is_floating_point_v<T>)
            static_assert(std::numeric_limits<T>::is_iec559,
                "[eser] floating-point deserialization requires an IEEE-754 (iec559) representation");
        T value {};
        if constexpr (std::is_same_v<T, bool>) {
            // Read through an unsigned char and normalize: a wire byte other than 0/1 must not be
            // materialized as a bool object, because a bool with a non-0/1 representation is a trap
            // value and reading it is undefined behavior. Any non-zero byte deserializes to `true`.
            // (This covers standalone `to<bool>()` and `bool` tuple elements; a `bool` nested inside
            // a struct or std::array is copied wholesale and is NOT normalized — see the class docs.)
            unsigned char raw = 0;
            std::memcpy(&raw, _data, sizeof(raw));
            value = (raw != 0);
        } else {
            std::memcpy(&value, _data, sizeof(T));
        }
        _data += sizeof(T);
        _length -= sizeof(T);
        utils::apply_wire_endianness<Wire>(value); // convert from wire order to host order
        return value;
    }

    template<endianness Wire>
    constexpr deserializer<Wire>::deserializer(const std::byte *data, std::size_t length)
    : _data(data), _length(length)
    {
    }

    template<endianness Wire>
    constexpr deserializer<Wire> deserialize(const std::byte *data, std::size_t length)
    {
        assert(data != nullptr && "Data pointer is null");
        return deserializer<Wire>(data, length);
    }
    template<endianness Wire>
    constexpr deserializer<Wire> deserialize(const std::uint8_t *data, std::size_t length)
    {
        return deserialize<Wire>(static_cast<const std::byte *>(static_cast<const void *>(data)), length);
    }
} // namespace eser::flat

#endif // ESER_FLAT_DESERIALIZER_TPP_