/**
* @file utils.hpp
* @ingroup ser_tools
* @brief Provides compile-time utilities for serialization size calculation.
*
* This header defines type traits and utility functions to compute the
* number of bytes required to serialize one or more types into a binary stream.
*
* These utilities are intended for use in serialization frameworks
* to enable buffer sizing without executing actual serialization code.
*
* @note This header is designed for extension and may include additional
*       utilities in the future.
* @author Mark Tikhonov <mtik.philosopher@gmail.com>
* @date 2025-07-02
* @copyright
* Creative Commons Attribution-NoDerivatives 4.0 International Public License
* See https://creativecommons.org/licenses/by-nd/4.0/
* SPDX-License-Identifier: CC-BY-ND-4.0
*/

#ifndef SER_TOOLS_UTILS_HPP_
#define SER_TOOLS_UTILS_HPP_
#include <type_traits>
#include <cstddef>
#include "traits.hpp"

namespace ser::tools
{
    
    /**
    * @brief Computes the serialized size (in bytes) of a single type T.
    *
    * Calculates, at compile time, how many bytes would be required
    * to serialize a single object of type T into a binary stream.
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
            static_assert(traits::always_false_v<bare_t>,
                "Runtime const char* cannot be used for compile-time size calculation. "
                "Use fixed-size char arrays or std::string_view instead.");
                return 0;
            }
            else {
                static_assert(traits::always_false_v<bare_t>,
                    "Unsupported type for serialization size calculation.");
                    return 0;
                }
            }
            
            /**
            * @brief Computes the total serialized size (in bytes) of multiple types.
            *
            * Computes, at compile time, how many bytes would be required
            * to serialize a sequence of types T... into a binary stream.
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
            
        } // namespace ser::tools
        
        #endif // SER_TOOLS_UTILS_HPP_
        