/**
* @file deserializer.hpp
*
* @brief Provides a deserialization utility for reconstructing objects and arrays from byte streams.
*
* @ingroup eser_flat eser::flat
*
* This file defines the `eser::flat::deserializer` class, which allows efficient deserialization
* of data from a byte array into standard C++ types such as integers, floating-point numbers,
* and arrays. The wire byte order is a compile-time policy (`Wire`, little-endian by default).
*
* @note The deserializer is not thread-safe.
* @note The wire byte order is selected via the `Wire` template parameter; see endianness.hpp.
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
*       Every `to()` overload now returns `std::optional<...>` and is `noexcept`:
*       it yields `std::nullopt` when the buffer holds fewer than the required
*       bytes, and the engaged value otherwise. The silent zero-fill ("best-fill")
*       behavior was removed.
*
*       `to()` became single-parameter ("you get the type you name"): the variadic
*       form was dropped. Read several fields by naming a `std::tuple`, e.g.
*       `to<std::tuple<int, float>>()`. The scalar and trivially-copyable-struct
*       overloads were merged into one trivially-copyable, non-array, non-tuple
*       overload.
*
*       Added the `Wire` endianness policy: `deserializer<Wire>` byte-reverses scalars when
*       `Wire` differs from the host order; structs are restricted to native-endian wires.
*/
#ifndef ESER_FLAT_DESERIALIZER_HPP_
#define ESER_FLAT_DESERIALIZER_HPP_
#include "../internal/byte.hpp"
#include "../internal/traits.hpp"
#include "../utils/endianness.hpp"
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <tuple>
#include <array>
#include <optional>

namespace eser::flat{
    using utils::endianness;

    /**
    * @class deserializer
    * @brief A utility class for deserializing data from a byte stream.
    *
    * The deserializer supports reconstructing multiple types and arrays from a raw byte stream.
    * It ensures that data is correctly interpreted and converted into the target types.
    * 
    * @note creation of deserilizer objects should be done through the `deserialize` function.
    * @note using methods `to` shifts the internal pointer by the number of bytes read,
    * so full deserialization invalidates the deserializer object.
    * @note `to()` is single-parameter: you get the type you name. To read several fields
    * in one call, name a `std::tuple`, e.g. `to<std::tuple<int, float>>()`.
    *
    * @warning **Untrusted input.** This is a tagless codec: `to<T>()` reconstructs `T` from raw
    *          bytes and only validates *length* (returning `std::nullopt` when too short). It does
    *          **not** validate semantic invariants — that is the protocol layer's responsibility.
    *          When parsing data from another device, mind the following:
    *          - **Length is trusted.** Never construct a deserializer with a `length` larger than
    *            the bytes you physically hold. A wire-supplied length (e.g. from a length prefix)
    *            must be validated against the number of bytes actually received first, or `to<T>()`
    *            will read out of bounds.
    *          - **Pointers.** Never deserialize a type containing a pointer member — you would
    *            reconstruct a raw pointer from attacker-controlled bytes.
    *          - **`bool` in aggregates.** A standalone `to<bool>()` (and a `bool` tuple element) is
    *            normalized so an out-of-range byte cannot form a trap value. A `bool` nested inside
    *            a struct or `std::array` is copied wholesale and is **not** normalized; only feed
    *            such types trusted/validated streams.
    *
    * @tparam Wire The byte order of the stream being read (default `endianness::little`). When it
    *              differs from the host order, scalar fields are byte-reversed; trivially-copyable
    *              structs may then not be read (raw bytes cannot be swapped).
    */
    template<endianness Wire = endianness::little>
    class deserializer{
    public:
        /**
        * @brief Deserialize a `std::tuple` of values from the byte stream.
        *
        * Reconstructs each tuple element in order. This is the way to read several fields
        * in a single call: name the fields as the tuple's element types. Arrays are read by
        * naming `std::array` elements (not C-arrays, which are not valid tuple members).
        *
        * @tparam Tuple A `std::tuple<Es...>` whose elements are each deserializable
        *               (scalar, enum, `std::array`, or trivially-copyable struct).
        * @return `std::nullopt` if the buffer holds fewer than the required bytes
        *         (`sizeof(Es) + ...`); otherwise an engaged optional holding the tuple.
        */
        template<typename Tuple, std::enable_if_t<internal::is_tuple_v<Tuple>, bool> = true>
        [[nodiscard]] std::optional<Tuple> to() noexcept;

        /**
        * @brief Deserialize a single trivially-copyable, non-array, non-tuple value.
        *
        * Reconstructs one value of the named type from the byte stream by copying its bytes.
        * This single overload covers scalars, enums, `std::array`, and trivially-copyable
        * structs/PODs — every type that is safe to copy with `memcpy` and has a fixed layout.
        *
        * To read a fixed-size array, name a `std::array<U, N>` (C-arrays are intentionally not
        * accepted: they cannot be returned by value and cannot be `std::tuple` elements).
        *
        * @tparam T The type to deserialize. Must be:
        *          - `std::is_trivially_copyable_v<T> == true`
        *          - not a C-array type (name a `std::array` instead)
        *          - not a `std::tuple` (use the tuple overload)
        *
        * @return `std::nullopt` if the buffer holds fewer than `sizeof(T)` bytes;
        *         otherwise an engaged optional holding the deserialized value of type `T`.
        *
        * @code
        * struct point { int x, y; };
        * static_assert(std::is_trivially_copyable_v<point>);
        *
        * auto p = deserializer.to<point>();  // Reads 8 bytes (2 ints) if available
        * @endcode
        */
        template<typename T, std::enable_if_t<
            std::is_trivially_copyable_v<T> &&
            !std::is_array_v<T> &&
            !internal::is_tuple_v<T>, bool> = true>
        [[nodiscard]] std::optional<T> to() noexcept;

        

    private:
        const std::byte *_data;       ///< Pointer to the byte stream.
        std::size_t _length;          ///< Length of the remaining data in the byte stream.
        
        /**
        * @brief Friend function to create a `deserializer` instance.
        *
        * This friend function allows the `deserialize` function to access the private constructor.
        *
        * @param data Pointer to the byte stream.
        * @param length Length of the byte stream.
        * @return A `deserializer` instance.
        */
        template<endianness W>
        friend constexpr deserializer<W> deserialize(const std::byte *data, std::size_t length);

        /**
        * @brief Internal method to copy a single trivially-copyable value off the stream.
        *
        * Reads `sizeof(T)` bytes and advances the cursor. Used for scalars, enums,
        * `std::array`, structs, and each element of a tuple. The caller guarantees the bytes
        * exist (the public `to()` overloads pre-check the length).
        *
        * @tparam T The trivially-copyable type to read.
        * @return The deserialized value.
        */
        template<typename T>
        T deserialize_impl() noexcept;

        /**
        * @brief Tuple back-end for `to<std::tuple<Es...>>()`.
        *
        * Unwraps the tuple's element pack so the bytes-required check and the per-element
        * reads can be expressed as fold expressions. The `type_identity` tag carries the
        * tuple type without constructing one.
        *
        * @tparam Es The tuple's element types.
        * @return `std::nullopt` if the buffer is too short; otherwise the engaged tuple.
        */
        template<typename... Es>
        std::optional<std::tuple<Es...>> to_impl(internal::type_identity<std::tuple<Es...>>) noexcept;

        /**
        * @brief Construct a deserializer.
        *
        * @param data Pointer to the byte stream.
        * @param length Length of the byte stream.
        */
        constexpr explicit deserializer(const std::byte *data, std::size_t length);
    };
    
    /**
    * @brief Create a deserializer instance from a byte array.
    *
    * This function constructs a `deserializer` object, enabling deserialization of data from the byte stream.
    *
    * @param data Pointer to the byte stream.
    * @param length Length of the byte stream.
    * @tparam Wire The byte order of the stream (default `endianness::little`). Provide it
    *              explicitly to read a big-endian stream, e.g. `deserialize<endianness::big>(...)`.
    * @return A `deserializer` instance initialized with the provided byte stream.
    * @note The function does not take ownership of the byte stream.
    * @warning `length` is trusted and bounds every subsequent `to<T>()` read. Pass the number of
    *          bytes you actually hold — never a larger value, and in particular never a length read
    *          from the wire without first validating it against the bytes received, or reads will
    *          run past the buffer.
    * @warning `data` must be non-null; this is checked by `assert` (debug builds only) and is a
    *          caller precondition, not a wire condition.
    */
    template<endianness Wire = endianness::little>
    constexpr deserializer<Wire> deserialize(const std::byte *data, std::size_t length);

    /**
    * @brief Create a deserializer instance from a compile time byte array.
    * @tparam Wire The byte order of the stream (default `endianness::little`).
    * @tparam N The length of the byte array.
    * @param data The byte array.
    * @return A `deserializer` instance initialized with the provided byte array.
    * @throws Assertion failure if the byte array pointer is null.
    * @note The function does not take ownership of the byte array.
    */
    template <endianness Wire = endianness::little, size_t N>
    constexpr deserializer<Wire> deserialize(const std::byte (&data)[N])
    {
        return deserialize<Wire>(data, N);
    }

    /**
    * @brief Create a deserializer instance from a `uint8_t` byte array.
    *
    * This overload allows constructing a `deserializer` from a byte stream
    * represented as a `const uint8_t*` pointer, for backwards compatibility
    * with existing codebases that use `uint8_t` to represent raw byte data.
    *
    * Internally, this function forwards the call to the main `std::byte` overload.
    *
    * @param data Pointer to the byte stream, represented as `const uint8_t*`.
    * @param length The length of the byte stream in bytes.
    * @return A `deserializer` instance initialized with the provided byte stream.
    *
    * @throws Assertion failure if the byte stream pointer is null.
    *
    * @note This function does not take ownership of the byte stream memory.
    * @note Prefer passing `std::byte*` to new code for clarity and modern C++17 compliance.
    *
    * @see deserialize(const std::byte*, std::size_t)
    */
    template<endianness Wire = endianness::little>
    constexpr deserializer<Wire> deserialize(const std::uint8_t *data, std::size_t length);

    /**
    * @brief Create a deserializer instance from a compile-time `uint8_t` byte array.
    *
    * This overload allows constructing a `deserializer` from a byte array
    * declared as a fixed-size `uint8_t` array, ensuring compatibility with
    * codebases that still use `uint8_t` as a byte representation.
    *
    * Internally, this function forwards the call to the main `std::byte` overload.
    *
    * @tparam N The size of the byte array.
    * @param data Reference to a compile-time `uint8_t` byte array.
    * @return A `deserializer` instance initialized with the provided byte array.
    *
    * @throws Assertion failure if the byte array pointer is null.
    *
    * @note This function does not take ownership of the byte array memory.
    * @note Prefer passing `std::byte` arrays to new code for clarity and modern C++17 compliance.
    *
    * @see deserialize(const std::byte*, std::size_t)
    * @see deserialize(const std::byte(&)[N])
    */
    template <endianness Wire = endianness::little, size_t N>
    constexpr deserializer<Wire> deserialize(const std::uint8_t (&data)[N])
    {
        return deserialize<Wire>(data, N);
    }
} // namespace eser::flat

#include "deserializer.tpp"
#endif // ESER_FLAT_DESERIALIZER_HPP_