/**
* @file deserializer.hpp
* @brief Provides a deserialization utility for reconstructing objects and arrays from byte streams.
*
* This file defines the `ser::binary::deserializer` class, which allows efficient deserialization
* of data from a byte array into standard C++ types such as integers, floating-point numbers,
* and arrays. The deserialization process assumes little-endian encoding.
*
* @note The deserializer is not thread-safe.
* @note Currently supports only little-endian transformations.
*/
#ifndef SER_BINARY_DESERIALIZER_HPP_
#define SER_BINARY_DESERIALIZER_HPP_

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <tuple>
#include <array>

namespace ser::binary{
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
    */
    class deserializer{
        public:
        /**
        * @brief Deserialize multiple values from the byte stream.
        *
        * This method reconstructs multiple values of specified types from the byte stream.
        * If one of the types is an array, it will be deserialized as a `std::array`.
        *
        * @tparam T... The types to deserialize. Can include arrays.
        * @return A tuple containing deserialized values, with arrays represented as `std::array`.
        * @throws Assertion failure if the byte stream does not contain sufficient data.
        * @note In release mode, the method will fill the remaining types with zeros if the data 
        * length is insufficient.
        */
        template<typename ...T, std::enable_if_t<(sizeof...(T) > 1), bool> = true>
        std::tuple<std::conditional_t<std::is_array_v<T>, std::array<std::remove_extent_t<T>, std::extent_v<T>>, T>...> 
        to();
        
        /**
        * @brief Deserialize a single array from the byte stream.
        *
        * This method reconstructs an array of the specified type from the byte stream.
        *
        * @tparam Vector The array type to deserialize.
        * @return A `std::array` containing the deserialized values.
        * @throws Assertion failure if the byte stream does not contain sufficient data for the array.
        * @note In release mode, the method will fill the remaining array elements with zeros if the data
        */
        template <typename Vector, std::enable_if_t<std::is_array_v<Vector>, bool> = true>
        std::array<std::remove_extent_t<Vector>, std::extent_v<Vector>> 
        to(); 
        
        /**
        * @brief Deserialize a single value from the byte stream.
        *
        * This method reconstructs a single value of the specified type from the byte stream.
        *
        * @tparam Scalar A scalar type to deserialize.
        * @return The deserialized value.
        * @throws Assertion failure if the byte stream does not contain sufficient data for the value.
        */
        template<typename Scalar, std::enable_if_t<std::is_scalar_v<Scalar>, bool> = true>
        Scalar to();
        
        private:
        const std::uint8_t *_data;    ///< Pointer to the byte stream.
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
        friend constexpr deserializer deserialize(const std::uint8_t *data, std::size_t length);
        
        /**
        * @brief Internal method to deserialize a single value.
        *
        * This method reconstructs a single value from the byte stream.
        *
        * @tparam T The type to deserialize.
        * @return The deserialized value.
        */
        template<typename T>
        std::enable_if_t<not std::is_array_v<T>, T> deserialize_impl();
        
        /**
        * @brief Internal method to deserialize an array.
        *
        * This method reconstructs an array of the specified type from the byte stream.
        *
        * @tparam T The array type to deserialize.
        * @return A `std::array` containing the deserialized values.
        */
        template<typename T>
        std::enable_if_t<std::is_array_v<T>, std::array<std::remove_extent_t<T>, std::extent_v<T>>> 
        deserialize_impl();
        
        /**
        * @brief Construct a deserializer.
        *
        * @param data Pointer to the byte stream.
        * @param length Length of the byte stream.
        */
        constexpr explicit deserializer(const std::uint8_t *data, std::size_t length);
    };
    
    /**
    * @brief Create a deserializer instance from a byte array.
    *
    * This function constructs a `deserializer` object, enabling deserialization of data from the byte stream.
    *
    * @param data Pointer to the byte stream.
    * @param length Length of the byte stream.
    * @return A `deserializer` instance initialized with the provided byte stream.
    * @throws Assertion failure if the byte stream pointer is null.
    * @note The function does not take ownership of the byte stream.
    */
    constexpr deserializer deserialize(const std::uint8_t *data, std::size_t length);
    
    /**
    * @brief Create a deserializer instance from a compile time byte array.
    * @tparam Length The length of the byte array.
    * @param data The byte array.
    * @return A `deserializer` instance initialized with the provided byte array.
    * @throws Assertion failure if the byte array pointer is null.
    * @note The function does not take ownership of the byte array.
    */
    template <size_t N>
    constexpr deserializer deserialize(const std::uint8_t (&data)[N])
    {
        return deserialize(data, N);
    }
} // namespace ser::binary

#include "deserializer.tpp"

#endif // SER_BINARY_DESERIALIZER_HPP_