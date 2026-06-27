/**
* @file size.hpp
*
* @ingroup eser_flat
*
* @brief Compile-time wire size of the flat (tagless) serialization format.
*
* `serialized_size_of<Ts...>()` returns the exact number of bytes the given types occupy on the
* wire in the flat format, so buffers can be sized at compile time. It is intentionally part of the
* `eser::flat` layer rather than a general utility: the size it reports is only meaningful for this
* tagless, fixed-layout format (a tagged or length-prefixed format would have a different size).
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
* -     Initial creation (as tools/utils.hpp).
* - 2026-06-24
* -     Moved out of the former `tools/utils.hpp` into `eser::flat` (flat/size.hpp);
*       the computed size is specific to the flat format.
*/
#ifndef ESER_FLAT_SIZE_HPP_
#define ESER_FLAT_SIZE_HPP_
#include <type_traits>
#include <cstddef>
#include "../internal/traits.hpp"

namespace eser::flat
{
    /**
    * @brief Computes the serialized size (in bytes) of a single type T.
    *
    * Calculates, at compile time, how many bytes would be required
    * to serialize a single object of type T into a flat byte stream.
    *
    * Supported types include:
    * - Arithmetic types (e.g. int, float)
    * - Enums
    * - C-style arrays (e.g. int[4])
    * - Trivially copyable structs/classes
    *
    * @tparam T The type whose serialized size is to be computed.
    * @return The size in bytes required to serialize T.
    *
    * @note This function is constexpr and will fail to compile
    *       for types that cannot be serialized at compile time.
    *
    * @warning Serialization size calculation for runtime `const char*`
    *          is not constexpr-safe prior to C++23. Use fixed-size arrays
    *          instead.
    */
    template<typename T>
    constexpr std::size_t serialized_size_of()
    {
        using bare_t = std::remove_cv_t<std::remove_reference_t<T>>;
        if constexpr (std::is_arithmetic_v<bare_t>) {
            return sizeof(bare_t);
        }
        else if constexpr (std::is_enum_v<bare_t>) {
            return sizeof(std::underlying_type_t<bare_t>);
        }
        else if constexpr (std::is_array_v<bare_t>) {
            return sizeof(bare_t);
        }
        else if constexpr (std::is_class_v<bare_t> && std::is_trivially_copyable_v<bare_t>) {
            return sizeof(bare_t);
        }
        else if constexpr (std::is_same_v<std::decay_t<bare_t>, const char*>) {
            static_assert(internal::always_false_v<bare_t>,
                "Runtime const char* cannot be used for compile-time size calculation. "
                "Use fixed-size char arrays or std::string_view instead.");
            return 0;
        }
        else {
            static_assert(internal::always_false_v<bare_t>,
                "Unsupported type for serialization size calculation.");
                return 0;
        }
    }

    /**
    * @brief Computes the total serialized size (in bytes) of multiple types.
    *
    * Computes, at compile time, how many bytes would be required
    * to serialize a sequence of types T... into a flat byte stream.
    *
    * This function aggregates the individual sizes of each type.
    *
    * @tparam T... The types whose total serialized size is to be computed.
    * @return The total size in bytes required to serialize all types in T...
    *
    * @note This function is constexpr and evaluates entirely at compile time
    *       for supported types.
    *
    * @see serialized_size_of<T>()
    */
    template<typename... T, std::enable_if_t<(sizeof...(T) > 1), bool> = true>
    constexpr std::size_t serialized_size_of()
    {
        return (... + serialized_size_of<T>());
    }

} // namespace eser::flat

#endif // ESER_FLAT_SIZE_HPP_
