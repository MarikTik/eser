/**
* @file serializer.tpp
* @author Mark Tikhonov <mtik.philosopher@gmail.com>
* @date 2025-07-02
* @copyright
* Creative Commons Attribution-NoDerivatives 4.0 International Public License
* See https://creativecommons.org/licenses/by-nd/4.0/
* SPDX-License-Identifier: CC-BY-ND-4.0
*/
#ifndef ESER_BINARY_SERIALIZER_TPP_
#define ESER_BINARY_SERIALIZER_TPP_
#include "serializer.hpp"
#include "../tools/traits.hpp"
#include "../tools/utils.hpp"
//#include <string_view>
namespace eser::binary{
    namespace __details{
        template <typename Vector, std::enable_if_t<std::is_array_v<Vector>, bool>>
        std::size_t serialize_impl(std::byte *&buffer, std::size_t &size, const Vector& vector)
        {    
            //assert(size >= sizeof(Vector) && "Buffer size potentially insufficient for the entire array");
            using type = std::remove_extent_t<Vector>;
            constexpr std::size_t N = std::extent_v<Vector>;
            std::size_t total_bytes = 0;
            
            for (std::size_t i = 0; i < N; ++i) {
                std::size_t bytes = serialize_impl(buffer, size, vector[i]);
                
                if (bytes == 0 && sizeof(type) > 0){
                    assert(false && "Buffer ran out during array element serialization");
                    break;
                }
                total_bytes += bytes;  
            }
            return total_bytes;  
        }
        
        template<typename Scalar, std::enable_if_t<std::is_arithmetic_v<Scalar>, bool>>
        std::size_t serialize_impl(std::byte *&buffer, std::size_t &size, Scalar scalar)
        {
            constexpr std::size_t scalar_size = sizeof(Scalar);
            //assert(scalar_size <= size && "Buffer size is insufficient for the scalar value");
            // if (size < scalar_size) {// possible error handling here}
            std::memcpy(static_cast<void*>(buffer), &scalar, scalar_size);
            buffer += scalar_size, size -= scalar_size;
            return scalar_size;
        }
        
        template<typename Enum, std::enable_if_t<std::is_enum_v<Enum>, bool>>
        std::size_t serialize_impl(std::byte *&buffer, std::size_t &size, Enum enum_member)
        {
            return serialize_impl(buffer, size, static_cast<std::underlying_type_t<Enum>>(enum_member));
        }
        
        template<
        typename Struct, 
        std::enable_if_t<
        std::is_class_v<Struct> and
        std::is_trivially_copyable_v<Struct>, bool
        > 
        >
        std::size_t serialize_impl(std::byte *&buffer, std::size_t &size, const Struct &str){
            constexpr std::size_t struct_size = sizeof(Struct);
            //assert(struct_size <= size && "Buffer size is insufficient for the struct");
            std::memcpy(static_cast<void*>(buffer), &str, struct_size);
            buffer += struct_size, size -= struct_size;
            return struct_size;
        }
        
        inline std::size_t serialize_impl(std::byte *&buffer, std::size_t &size, const char *str)
        {
            std::size_t length = std::strlen(str) + 1; // +1 for null terminator (The string MUST be null-terminated)
            //assert(size >= length && "Buffer size is insufficient for the string");
            std::memcpy(static_cast<void*>(buffer), str, length);
            buffer += length, size -= length;
            return length;
        }
    } // namespace __details
    
    template <typename... T>
    inline std::size_t serializer<T...>::to (std::byte *buffer, std::size_t size) const
    {
        using namespace __details;
        constexpr std::size_t needed = tools::serialized_size_of<T...>();
        if (needed > size){
            assert(false && "Buffer size is insufficient for serialization");
            return 0; 
        }
        return std::apply([&](const auto &...args){
            return (... + serialize_impl(buffer, size, args));
        }, _args);
    }

    template <typename... T>
    std::size_t serializer<T...>::to(std::uint8_t *buffer, std::size_t size) const
    {
        return to(static_cast<std::byte *>(static_cast<void*>(buffer)), size);
    }
    template <typename... T>
    template <size_t N>
    inline std::size_t serializer<T...>::to(std::byte (&buffer)[N]) const
    {
        return to(buffer, N);
    }

    template <typename... T>
    template <size_t N>
    std::size_t serializer<T...>::to(std::uint8_t (&buffer)[N]) const
    {
        return to(static_cast<std::byte *>(static_cast<void *>(buffer)), N);
    }

    template <typename... T>
    constexpr serializer<T...>::serializer(T &&...args)
    : _args(std::forward<T>(args)...)
    {
    }
} // namespace eser::binary
    
#endif // ESER_BINARY_SERIALIZER_TPP_