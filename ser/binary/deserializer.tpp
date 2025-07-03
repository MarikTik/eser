/**
* @file deserializer.tpp
* @author Mark Tikhonov <mtik.philosopher@gmail.com>
* @date 2025-07-02
* @copyright
* Creative Commons Attribution-NoDerivatives 4.0 International Public License
* See https://creativecommons.org/licenses/by-nd/4.0/
* SPDX-License-Identifier: CC-BY-ND-4.0
*/
#ifndef SER_BINARY_DESERIALIZER_TPP_
#define SER_BINARY_DESERIALIZER_TPP_
#include "deserializer.hpp"
#include <cassert>
#include <utility>
#include <array>
#include <cstring>
namespace ser::binary{
    template <typename... T, std::enable_if_t<(sizeof...(T) > 1), bool>>
    inline std::tuple<std::conditional_t<std::is_array_v<T>, std::array<std::remove_extent_t<T>, std::extent_v<T>>, T>...> 
    deserializer::to()
    {
        constexpr std::size_t bytes_required = (sizeof(T) + ...);
        static_assert(sizeof...(T) > 0, "At least one type must be specified");
        static_assert((std::is_trivially_constructible_v<T> and ...), "All types must be trivially constructible");
        assert(_length >= bytes_required && "Data length is insufficient for the requested types"); 
        return { deserialize_impl<T>()... };
    }
    
    template <typename Vector, std::enable_if_t<std::is_array_v<Vector>, bool>>
    inline std::array<std::remove_extent_t<Vector>, std::extent_v<Vector>> deserializer::to()
    {
        assert(_length >= sizeof(Vector) && "Data length is insufficient for the requested array type");
        return deserialize_impl<Vector>();
    }
    
    template <typename Scalar, std::enable_if_t<std::is_scalar_v<Scalar>, bool>>
    inline Scalar deserializer::to()
    {
        assert(sizeof(Scalar) <= _length && "Data length is insufficient for the requested type");
        auto result = deserialize_impl<Scalar>();
        return result;
    }
    
    template<typename T>
    std::enable_if_t<std::is_array_v<T>, std::array<std::remove_extent_t<T>, std::extent_v<T>>>
    deserializer::deserialize_impl()
    {
        using type = std::remove_extent_t<T>;
        constexpr std::size_t N = std::extent_v<T>;
        constexpr std::size_t type_size = sizeof(type);
        static_assert(N > 0, "Array extent (size) must be greater than zero");
        
        std::array<type, N> array;
        std::size_t i = 0;
        size_t length = _length; // save the length before we start modifying it
        for (; i < N and (i + 1) * type_size <= length; i++)
        array[i] = deserialize_impl<type>();
        
        if (i not_eq N){
            std::fill(array.begin() + i, array.end(), type{});
            // flag an esp32 exception here
        }
        return array;
    }
    
    template<typename T>
    std::enable_if_t<not std::is_array_v<T>, T> deserializer::deserialize_impl()
    { 
        constexpr std::size_t type_size = sizeof(T);
        if (_length < type_size){
            // flag an esp32 exception here or plan on using std::optional<T> 
            // to indicate that the value is not available
            return T{}; 
        }
        T value {};
        std::memcpy(&value, _data, type_size);
        _data += type_size;
        _length -= type_size;         
        return value;
    }
    
    constexpr deserializer::deserializer(const std::byte *data, std::size_t length)
    : _data(data), _length(length)
    {
    }
    
    constexpr deserializer deserialize(const std::byte *data, std::size_t length)
    {
        assert(data != nullptr && "Data pointer is null");
        return deserializer(data, length);
    }
    constexpr deserializer deserialize(const std::uint8_t *data, std::size_t length)
    {
        return deserialize(static_cast<const std::byte *>(static_cast<const void *>(data)), length);
    }
} // namespace ser::binary

#endif // SER_BINARY_DESERIALIZER_TPP_