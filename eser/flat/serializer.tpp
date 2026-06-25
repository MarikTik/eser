/**
* @file serializer.tpp
*
* @brief Definition of functionality in serialzer.hpp
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
*       Added the `Wire` endianness policy: `serializer<Wire, T...>` byte-reverses scalars when
*       `Wire` differs from the host order; structs are restricted to native-endian wires.
*/
#ifndef ESER_FLAT_SERIALIZER_TPP_
#define ESER_FLAT_SERIALIZER_TPP_
#include "serializer.hpp"
#include "../tools/traits.hpp"
#include "../tools/utils.hpp"
#include "../tools/endianness.hpp"
#include <limits>
namespace eser::flat{
    namespace details{
        template <endianness Wire, typename Array, std::enable_if_t<tools::is_std_array_v<Array>, bool>>
        std::size_t serialize_impl(std::byte *&buffer, std::size_t &size, const Array& array)
        {
            std::size_t total_bytes = 0;
            for (const auto& element : array) {
                std::size_t bytes = serialize_impl<Wire>(buffer, size, element);

                if (bytes == 0 && sizeof(typename Array::value_type) > 0){
                    assert(false && "Buffer ran out during array element serialization");
                    break;
                }
                total_bytes += bytes;
            }
            return total_bytes;
        }

        template<endianness Wire, typename Scalar, std::enable_if_t<std::is_arithmetic_v<Scalar>, bool>>
        std::size_t serialize_impl(std::byte *&buffer, std::size_t &size, Scalar scalar)
        {
            if constexpr (std::is_floating_point_v<Scalar>)
                static_assert(std::numeric_limits<Scalar>::is_iec559,
                    "[eser] floating-point serialization requires an IEEE-754 (iec559) representation");
            constexpr std::size_t scalar_size = sizeof(Scalar);
            //assert(scalar_size <= size && "Buffer size is insufficient for the scalar value");
            tools::apply_wire_endianness<Wire>(scalar); // by-value copy; convert to wire order
            std::memcpy(static_cast<void*>(buffer), &scalar, scalar_size);
            buffer += scalar_size, size -= scalar_size;
            return scalar_size;
        }

        template<endianness Wire, typename Enum, std::enable_if_t<std::is_enum_v<Enum>, bool>>
        std::size_t serialize_impl(std::byte *&buffer, std::size_t &size, Enum enum_member)
        {
            return serialize_impl<Wire>(buffer, size, static_cast<std::underlying_type_t<Enum>>(enum_member));
        }

        template<
        endianness Wire,
        typename Struct,
        std::enable_if_t<
        std::is_class_v<Struct> and
        std::is_trivially_copyable_v<Struct> and
        not tools::is_std_array_v<Struct>, bool
        >
        >
        std::size_t serialize_impl(std::byte *&buffer, std::size_t &size, const Struct &str){
            static_assert(Wire == tools::host_endianness or tools::is_endianness_neutral_v<Struct>,
                "[eser] trivially-copyable structs are serialized as raw bytes and cannot be "
                "byte-swapped; serialize with the native wire endianness, split the struct into "
                "scalar fields, or specialize is_endianness_neutral if the type is byte-only");
            constexpr std::size_t struct_size = sizeof(Struct);
            //assert(struct_size <= size && "Buffer size is insufficient for the struct");
            std::memcpy(static_cast<void*>(buffer), &str, struct_size);
            buffer += struct_size, size -= struct_size;
            return struct_size;
        }
    } // namespace details

    template <endianness Wire, typename... T>
    inline std::size_t serializer<Wire, T...>::to (std::byte *buffer, std::size_t size) const
    {
        using namespace details;
        constexpr std::size_t needed = tools::serialized_size_of<T...>();
        if (needed > size){
            assert(false && "Buffer size is insufficient for serialization");
            return 0;
        }
        return std::apply([&](const auto &...args){
            return (... + serialize_impl<Wire>(buffer, size, args));
        }, _args);
    }

    template <endianness Wire, typename... T>
    std::size_t serializer<Wire, T...>::to(std::uint8_t *buffer, std::size_t size) const
    {
        return to(static_cast<std::byte *>(static_cast<void*>(buffer)), size);
    }
    template <endianness Wire, typename... T>
    template <size_t N>
    inline std::size_t serializer<Wire, T...>::to(std::byte (&buffer)[N]) const
    {
        return to(buffer, N);
    }

    template <endianness Wire, typename... T>
    template <size_t N>
    std::size_t serializer<Wire, T...>::to(std::uint8_t (&buffer)[N]) const
    {
        return to(static_cast<std::byte *>(static_cast<void *>(buffer)), N);
    }

    template <endianness Wire, typename... T>
    constexpr serializer<Wire, T...>::serializer(T... args)
    : _args(std::move(args)...)
    {
    }
} // namespace eser::flat
    
#endif // ESER_FLAT_SERIALIZER_TPP_