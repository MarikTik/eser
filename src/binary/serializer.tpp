#ifndef SER_BINARY_SERIALIZER_TPP_
#define SER_BINARY_SERIALIZER_TPP_
#include "serializer.hpp"
#include "../traits.hpp"
#include <string_view>
namespace ser::binary{
    namespace __details{
        template <typename Vector, std::enable_if_t<std::is_array_v<Vector>, bool>>
        std::size_t serialize_impl(std::uint8_t *&buffer, std::size_t &size, const Vector& vector)
        {    
            assert(size >= sizeof(Vector) && "Buffer size potentially insufficient for the entire array");
            using type = std::remove_extent_t<Vector>;
            constexpr std::size_t N = std::extent_v<Vector>;
            std::size_t total_bytes = 0;
            
            for (std::size_t i = 0; i < N; ++i) {
                // Check remaining size *before* recursive call for this element
                // (Approximate check, actual size known after call returns)
                // if (size < sizeof(type)) { // Simple check, might be inaccurate
                //    assert(false && "Buffer ran out predicting array element serialization");
                //    break;
                // }
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
        std::size_t serialize_impl(std::uint8_t *&buffer, std::size_t &size, Scalar scalar)
        {
            constexpr std::size_t scalar_size = sizeof(Scalar);
            assert(scalar_size <= size && "Buffer size is insufficient for the scalar value");
            // if (size < scalar_size) {// possible error handling here}
            std::memcpy(buffer, &scalar, scalar_size);
            buffer += scalar_size, size -= scalar_size;
            return scalar_size;
        }
        
        template<typename Enum, std::enable_if_t<std::is_enum_v<Enum>, bool>>
        std::size_t serialize_impl(std::uint8_t *&buffer, std::size_t &size, Enum enum_member)
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
        std::size_t serialize_impl(std::uint8_t *&buffer, std::size_t &size, const Struct &str){
            constexpr std::size_t struct_size = sizeof(Struct);
            assert(struct_size <= size && "Buffer size is insufficient for the struct");
            std::memcpy(buffer, &str, struct_size);
            buffer += struct_size, size -= struct_size;
            return struct_size;
        }
        
        inline std::size_t serialize_impl(std::uint8_t *&buffer, std::size_t &size, const char *str)
        {
            std::size_t length = std::strlen(str) + 1; // +1 for null terminator (The string MUST be null-terminated)
            assert(size >= length && "Buffer size is insufficient for the string");
            std::memcpy(buffer, str, length);
            buffer += length, size -= length;
            return length;
        }
        
        template <typename T>
        constexpr std::size_t individual_serialized_size(const T& item) {
            // Use if constexpr for compile-time branching based on type
            if constexpr (std::is_arithmetic_v<T>) {
                return sizeof(T);
            } 
            else if constexpr (std::is_enum_v<T>) {
                return sizeof(std::underlying_type_t<T>);
            } 
            else if constexpr (std::is_array_v<T>) { // Handles C-style arrays T[N]
                std::size_t total_size = 0;
                for (const auto& element : item) {
                    total_size += individual_serialized_size(element); // Use helper recursively
                }
                return total_size;
            } 
            else if constexpr (std::is_class_v<T> && std::is_trivially_copyable_v<T>) {
                // Note: std::array is trivially copyable if T is, but handled above.
                return sizeof(T); // Assumes memcpy includes padding
            } 
            else if constexpr (std::is_same_v<T, std::string_view>) {
                return item.size(); // Size of content
            }
            else if constexpr (std::is_same_v<std::decay_t<T>, const char*>) {
                // Cannot make constexpr with runtime strlen pre-C++23
                static_assert(!std::is_same_v<std::decay_t<T>, const char*>,
                    "Calculating serialized size of runtime const char* is not constexpr safe pre-C++23. Use fixed-size char arrays or std::string_view.");
                    return 0;
                }
                else {
                    static_assert(ser::traits::always_false_v<T>, "Unsupported type for constexpr serialized size calculation.");
                    return 0;
                }
        }
    } // namespace __details
    
    template <typename... T>
    inline std::size_t serializer<T...>::to (std::uint8_t *buffer, std::size_t size) const
    {
        using namespace __details;
        
        return std::apply([&](const auto &...args){
            return (... + serialize_impl(buffer, size, args));
        }, _args);
    }
    
    template <typename... T>
    template <size_t N>
    inline std::size_t serializer<T...>::to(std::uint8_t (&buffer)[N]) const
    {
        return to(buffer, N);
    }
    
    
    template <typename... T>
    constexpr std::size_t serializer<T...>::size() const {
        return std::apply([](const auto&... args) {
            if constexpr (sizeof...(args) == 0) return 0;
            else return (... + __details::individual_serialized_size(args));
        }, _args);
    }
    
    template <typename... T>
    constexpr serializer<T...>::serializer(T &&...args)
    : _args(std::forward<T>(args)...)
    {
    }
} // namespace scr::utils
    
#endif // SER_BINARY_SERIALIZER_TPP_