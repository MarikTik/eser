/**
* @file serializer.hpp
*
* @brief Provides a utility class for converting C++ objects and arrays into raw byte streams.
*
* @ingroup eser_flat eser::flat
*
* This file defines the `eser::flat::serializer` class template, which allows efficient
* serialization of:
*
* - Scalar types (integers, floats, enums)
* - C-style arrays
* - `std::array`
* - trivially copyable structs
* - Null-terminated C strings
*
* The serialization process writes objects into a contiguous buffer of `std::byte` elements,
* suitable for storage, network transmission, or embedded communication protocols.
*
* ## Endianness
*
* The wire byte order is a compile-time policy (`Wire`, **little-endian by default**). Pass
* `endianness::big` to write network byte order. Scalars are byte-reversed via `if constexpr`
* only when the wire order differs from the host; raw structs are restricted to the native order.
*
* ## Buffer Safety
*
* The serializer does not own the output buffer memory. It is the caller's responsibility to:
*
* - Provide a buffer of sufficient size (use `serialized_size_of` helpers to determine size).
* - Ensure the lifetime of the buffer outlasts serialization operations.
*
* ## Thread Safety
*
* The serializer class is **not thread-safe** if:
*
* - Multiple threads invoke `.to()` on the same serializer instance concurrently.
* - Overlapping writes occur into the same output buffer.
*
* However, separate instances are independent and can safely serialize in parallel.
*
* ## Example Usage
*
* ```cpp
* #include "eser/flat/serializer.hpp"
*
* using namespace eser::flat;
*
* int a = 42;
* float b = 3.14f;
*
* auto s = serialize(a, b);
*
* std::byte buffer[64]{};
* size_t bytes_written = s.to(buffer);
* ```
*
* @note This file is part of the `ser_binary` Doxygen group.
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
*/
#ifndef ESER_FLAT_SERIALIZER_HPP_
#define ESER_FLAT_SERIALIZER_HPP_
#include <type_traits>
#include <tuple>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include "../tools/byte.hpp"
#include "../tools/endianness.hpp"
namespace eser::flat{
    using tools::endianness;

    namespace details{
        /**
        * @brief Internal method to serialize an array.
        *
        * This method serializes an array into the byte stream.
        *
        * @tparam Wire The byte order written to the stream.
        * @tparam Vector The array type to serialize.
        * @param buffer A pointer to the output byte stream.
        * @param size The remaining size of the output buffer.
        * @param vector The array to serialize.
        * @return The number of bytes written to the buffer.
        */
        template<endianness Wire, typename Vector, std::enable_if_t<std::is_array_v<Vector>, bool> = true>
        std::size_t serialize_impl(std::byte *&buffer, std::size_t &size, const Vector& vector);
        
        /**
        * @brief Internal method to serialize a scalar value.
        *
        * Serializes a single scalar value (integer, float, etc.) into the byte stream,
        * byte-reversed when `Wire` differs from the host byte order.
        *
        * @tparam Wire The byte order written to the stream.
        * @tparam Scalar A trivially copyable arithmetic type.
        * @param buffer A pointer to the output byte stream as `std::byte*`.
        * @param size Reference to the remaining size of the output buffer in bytes.
        * @param scalar The scalar value to serialize.
        * @return The number of bytes written to the buffer.
        *
        * @note The caller must ensure that `size` is at least `sizeof(Scalar)`.
        */
        template<endianness Wire, typename Scalar, std::enable_if_t<std::is_arithmetic_v<Scalar>, bool> = true>
        std::size_t serialize_impl(std::byte *&buffer, std::size_t &size, Scalar scalar);
        
        /**
        * @brief Internal method to serialize an enum value.
        *
        * This method serializes an enum value into the byte stream.
        *
        * @tparam Wire The byte order written to the stream.
        * @tparam Enum The enum type to serialize.
        * @param buffer A pointer to the output byte stream.
        * @param size The remaining size of the output buffer.
        * @param enum_member The enum value to serialize.
        * @return The number of bytes written to the buffer.
        */
        template<endianness Wire, typename Enum, std::enable_if_t<std::is_enum_v<Enum>, bool> = true>
        std::size_t serialize_impl(std::byte *&buffer, std::size_t &size, Enum enum_member);
        
        /**
        * @brief Internal method to serialize a trivially copyable struct.
        *
        * This method serializes a trivially copyable struct into the byte stream.
        *
        * @tparam Wire The byte order written to the stream. A struct may only be serialized when
        *              `Wire` equals the host byte order (raw bytes cannot be byte-swapped).
        * @tparam Struct The struct type to serialize.
        * @param buffer A pointer to the output byte stream.
        * @param size The remaining size of the output buffer.
        * @param str The struct to serialize.
        * @return The number of bytes written to the buffer.
        */
        template<
        endianness Wire,
        typename Struct,
        std::enable_if_t<
        std::is_class_v<Struct> and
        std::is_trivially_copyable_v<Struct>, bool
        > = true
        >
        std::size_t serialize_impl(std::byte *&buffer, std::size_t &size, const Struct &str);
    }
    /**
    * @class serializer
    * @brief A utility class for serializing multiple values into a byte stream.
    *
    * The serializer supports converting multiple types and arrays into a raw byte stream.
    * It ensures that data is correctly interpreted and converted into the byte stream.
    *
    * @tparam Wire The byte order written to the stream (default `endianness::little`). When it
    *              differs from the host order, scalar fields are byte-reversed; trivially-copyable
    *              structs may then not be serialized (raw bytes cannot be swapped).
    * @tparam T... The types held for serialization.
    */
    template<endianness Wire, typename ...T>
    class serializer{
    public:
        /**
        * @brief Serialize multiple values into a byte stream.
        *
        * Serializes the values held in the serializer instance into a contiguous
        * byte stream, starting at the memory location specified by `buffer`.
        *
        * The serialization process writes objects into the buffer in the `Wire` byte
        * order (little-endian by default). Scalars, arrays, enums, and trivially copyable
        * structs are all supported.
        *
        * ## Usage
        *
        * ```cpp
        * using namespace eser::flat;
        *
        * auto s = serialize(42, 3.14f);
        *
        * std::byte buffer[64]{};
        * size_t bytes_written = s.to(buffer, sizeof(buffer));
        * ```
        *
        * @param buffer A pointer to a writable output byte stream as `std::byte*`.
        *               The buffer must be large enough to hold the serialized data.
        * @param size The size of the output buffer in bytes.
        * @return The number of bytes written to the buffer.
        *
        * @note The method does not perform bounds checking beyond assertions
        *       in debug mode. Always ensure the provided buffer is large enough.
        *
        * @see serializer::to(std::byte (&)[N])
        */
        std::size_t to(std::byte *buffer, std::size_t size) const;
        
        /**
        * @brief Serialize multiple values into a fixed-size byte array.
        *
        * Serializes the values held in the serializer instance into a compile-time
        * sized array of `std::byte`. This overload simplifies usage when working
        * with static buffers, avoiding the need to explicitly specify the buffer
        * size as a parameter.
        *
        * ## Usage
        *
        * ```cpp
        * using namespace eser::flat;
        *
        * auto s = serialize(42, 3.14f);
        *
        * std::byte buffer[64]{};
        * size_t bytes_written = s.to(buffer);
        * ```
        *
        * @tparam N The size of the output array in bytes.
        * @param buffer A fixed-size writable array of `std::byte` elements
        *               that receives the serialized data.
        * @return The number of bytes written to the buffer.
        *
        * @note The method does not perform bounds checking beyond assertions
        *       in debug mode. Always ensure the array is large enough for
        *       all serialized data.
        *
        * @see serializer::to(std::byte*, std::size_t)
        */
        template<size_t N>
        std::size_t to(std::byte (&buffer)[N]) const;
        
        /**
        * @brief Serialize multiple values into a legacy `uint8_t` byte stream.
        *
        * This overload of `to()` provides backwards compatibility for projects
        * that represent byte buffers as `std::uint8_t*` rather than `std::byte*`.
        *
        * Internally, the method performs a safe cast to a `std::byte*` pointer
        * and forwards the call to the primary serialization logic.
        *
        * ## Usage
        *
        * ```cpp
        * using namespace eser::flat;
        *
        * auto s = serialize(42, 3.14f);
        *
        * std::uint8_t buffer[64]{};
        * size_t bytes_written = s.to(buffer, sizeof(buffer));
        * ```
        *
        * @param buffer A pointer to a legacy byte stream as `std::uint8_t*`.
        *               The buffer must be large enough to hold the serialized data.
        * @param size The size of the output buffer in bytes.
        * @return The number of bytes written to the buffer.
        *
        * @note Prefer using the modern overload that accepts `std::byte*`
        *       for improved type safety and clarity in modern C++ code.
        *
        * @see serializer::to(std::byte*, std::size_t)
        */
        std::size_t to(std::uint8_t *buffer, std::size_t size) const;
        
        
        /**
        * @brief Serialize multiple values into a legacy fixed-size `uint8_t` buffer.
        *
        * This overload of `to()` enables serialization into compile-time sized arrays
        * of legacy `std::uint8_t` bytes for backwards compatibility.
        *
        * Internally, the method safely casts the provided array to a `std::byte*` buffer
        * and forwards the call to the primary serialization logic.
        *
        * ## Usage
        *
        * ```cpp
        * using namespace eser::flat;
        *
        * auto s = serialize(42, 3.14f);
        *
        * std::uint8_t buffer[64]{};
        * size_t bytes_written = s.to(buffer);
        * ```
        *
        * @tparam N The size of the output array in bytes.
        * @param buffer A fixed-size array of legacy `std::uint8_t` bytes.
        * @return The number of bytes written to the buffer.
        *
        * @note Prefer using the modern overload that accepts a `std::byte` array
        *       for improved type safety and clarity in modern C++ code.
        *
        * @see serializer::to(std::byte (&)[N])
        */
        template<size_t N>
        std::size_t to(std::uint8_t (&buffer)[N]) const;
        
    private:
        std::tuple<T...> _args; ///< A tuple containing the values to serialize.
        
        
        /**
        * @brief Private constructor to prevent direct instantiation.
        *
        * This constructor is private to enforce the use of the `serialize` function.
        *
        * @param args The values to serialize.
        */
        constexpr explicit serializer(T&&... args);
        
        /**
        * @brief Friend function to create a serializer instance.
        *
        * This friend function allows the `serialize` function to access the private constructor.
        *
        * @tparam W The byte order to serialize with.
        * @tparam U... The types to serialize.
        * @param args The values to serialize.
        * @return A `serializer` instance.
        */
        template<endianness W, typename ...U>
        friend constexpr serializer<W, U...> serialize(U&&... args);
    };

    /**
    * @brief Factory function to create a serializer instance holding the given arguments.
    *
    * Constructs a `serializer<Wire, T...>` object, capturing the provided arguments
    * (usually by value or reference depending on perfect forwarding).
    *
    * @tparam Wire The byte order to serialize with (default `endianness::little`). Provide it
    *              explicitly to write a big-endian stream, e.g. `serialize<endianness::big>(a, b)`.
    * @tparam T... The deduced types of the arguments.
    * @param args The arguments to be serialized.
    * @return A `serializer` instance initialized with the provided arguments.
    */
    template <endianness Wire = endianness::little, typename... T>
    constexpr serializer<Wire, T...> serialize(T &&...args)
    {
        static_assert(sizeof...(T) > 0, "At least one type must be specified");
        return serializer<Wire, T...>(std::forward<T>(args)...);
    }
    
} // eser::flat

#include "serializer.tpp"
#endif // ESER_FLAT_SERIALIZER_HPP_