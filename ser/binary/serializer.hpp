/**
* @file serializer.hpp
* @brief Provides a serialization utility class for converting objects and arrays into byte streams.
*
* This file defines the `ser::binary::serializer` class template, which allows efficient serialization
* of standard C++ types (like integers, floats, enums), C-style arrays, std::array, std::string_view,
* and trivially copyable structs into a raw byte stream. It also provides a constexpr method
* to calculate the required serialization size beforehand.
* The serialization process currently assumes little-endian encoding where applicable.
*
* @note The serializer instance is immutable after creation via the `serialize` function.
* @note The serializer is not thread-safe if multiple threads call `.to()` on the same instance
* with overlapping buffers (though instances themselves are usually short-lived).
* @note Currently supports only little-endian transformations for multi-byte types.
*
* @author Mark Tikhonov <mtik.philosopher@gmail.com>
* @date 2025-07-02
* @copyright
* Creative Commons Attribution-NoDerivatives 4.0 International Public License
* See https://creativecommons.org/licenses/by-nd/4.0/
* SPDX-License-Identifier: CC-BY-ND-4.0
*/
#ifndef SER_BINARY_SERIALIZER_HPP_
#define SER_BINARY_SERIALIZER_HPP_
#include <type_traits>
#include <tuple>
#include <cassert>
#include <cstring>
#include <cstddef>
#include <cstdint>

namespace ser::binary{
    namespace __details{
        /**
        * @brief Internal method to serialize an array.
        *
        * This method serializes an array into the byte stream.
        *
        * @tparam Vector The array type to serialize.
        * @param buffer A pointer to the output byte stream.
        * @param size The remaining size of the output buffer.
        * @param vector The array to serialize.
        * @return The number of bytes written to the buffer.
        */
        template<typename Vector, std::enable_if_t<std::is_array_v<Vector>, bool> = true>
        std::size_t serialize_impl(std::uint8_t *&buffer, std::size_t &size, const Vector& vector);
        
        /**
        * @brief Internal method to serialize a scalar value.
        *
        * This method serializes a scalar value into the byte stream.
        *
        * @tparam Scalar The scalar type to serialize.
        * @param buffer A pointer to the output byte stream.
        * @param size The remaining size of the output buffer.
        * @param scalar The scalar value to serialize.
        * @return The number of bytes written to the buffer.
        */
        template<typename Scalar, std::enable_if_t<std::is_arithmetic_v<Scalar>, bool> = true>
        std::size_t serialize_impl(std::uint8_t *&buffer, std::size_t &size, Scalar scalar);
        
        /**
        * @brief Internal method to serialize an enum value.
        *
        * This method serializes an enum value into the byte stream.
        *
        * @tparam Enum The enum type to serialize.
        * @param buffer A pointer to the output byte stream.
        * @param size The remaining size of the output buffer.
        * @param enum_member The enum value to serialize.
        * @return The number of bytes written to the buffer.
        */
        template<typename Enum, std::enable_if_t<std::is_enum_v<Enum>, bool> = true>
        std::size_t serialize_impl(std::uint8_t *&buffer, std::size_t &size, Enum enum_member);
        
        /**
        * @brief Internal method to serialize a trivially copyable struct.
        *
        * This method serializes a trivially copyable struct into the byte stream.
        *
        * @tparam Struct The struct type to serialize.
        * @param buffer A pointer to the output byte stream.
        * @param size The remaining size of the output buffer.
        * @param str The struct to serialize.
        * @return The number of bytes written to the buffer.
        */
        template<
        typename Struct, 
        std::enable_if_t<
        std::is_class_v<Struct> and
        std::is_trivially_copyable_v<Struct>, bool
        > = true
        >
        std::size_t serialize_impl(std::uint8_t *&buffer, std::size_t &size, const Struct &str);
    }
    /**
    * @class serializer
    * @brief A utility class for serializing multiple values into a byte stream.
    *
    * The serializer supports converting multiple types and arrays into a raw byte stream.
    * It ensures that data is correctly interpreted and converted into the byte stream.
    */
    template<typename ...T>
    class serializer{
    public:
        /**
        * @brief Serialize multiple values into the byte stream.
        *
        * This method serializes multiple values of specified types into the byte stream.
        *
        * @param buffer A pointer to the output byte stream.
        * @param size The size of the output buffer.
        * @return The number of bytes written to the buffer.
        */
        std::size_t to(std::uint8_t *buffer, std::size_t size) const;
        
        /**
        * @brief Serialize multiple values into a compile-time sized buffer.
        *
        * This method serializes multiple values of specified types into a compile-time sized buffer.
        *
        * @tparam N The size of the buffer.
        * @param buffer The output buffer.
        * @return The number of bytes written to the buffer.
        */
        template<size_t N>
        std::size_t to(std::uint8_t(&buffer)[N]) const;
        
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
        * @tparam U... The types to serialize.
        * @param args The values to serialize.
        * @return A `serializer` instance.
        */
        template<typename ...U>
        friend constexpr serializer<U...> serialize(U&&... args);
    };
    
    /**
    * @brief Factory function to create a serializer instance holding the given arguments.
    *
    * Constructs a `serializer<T...>` object, capturing the provided arguments
    * (usually by value or reference depending on perfect forwarding).
    *
    * @tparam T... The deduced types of the arguments.
    * @param args The arguments to be serialized.
    * @return A `serializer` instance initialized with the provided arguments.
    */
    template <typename... T>
    constexpr serializer<T...> serialize(T &&...args)
    {
        static_assert(sizeof...(T) > 0, "At least one type must be specified");
        return serializer<T...>(std::forward<T>(args)...);
    }
    
} // ser::binary

#include "serializer.tpp"

#endif // SER_BINARY_SERIALIZER_HPP_