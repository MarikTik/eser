/**
* @file traits.hpp
*
* @brief Provides custom type traits for template metaprogramming.
*
* @ingroup eser_tools
*
* This file defines additional type traits outside of the C++ standard library.
* These traits are designed for use in modern template metaprogramming,
* following the same structure and naming conventions as standard library traits.
* 
* @note These traits reside in the `ser::traits` namespace to avoid collision
* with standard traits and other libraries.
* 
* @warning This file is intended to use on platforms supporting C++17 standard or later.
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
#ifndef ESER_TRAITS_HPP_
#define ESER_TRAITS_HPP_

#include <type_traits> // For std::true_type, std::bool_constant, std::is_same_v

namespace eser::tools {
    /**
    * @struct is_unique
    * @brief Checks whether a pack of types is composed of distinct types.
    *
    * This type trait determines at compile-time whether all types provided in the parameter pack
    * are unique (i.e., no duplicates). The check is performed recursively using fold expressions
    * and `std::is_same_v`.
    *
    * The result is accessible via the `::value` member or via the `is_unique_v` alias.
    *q
    * @tparam Ts The parameter pack of types to check for uniqueness.
    *
    * @note This trait is useful when implementing compile-time type sets or constraints where
    * repeated types would cause ambiguity or incorrect behavior.
    *
    * @see is_unique_v
    */
    template <typename...>
    struct is_unique : std::true_type {};
    
    /**
    * @brief Recursive specialization of `is_unique` to check for duplicate types.
    *
    * Evaluates whether the current type `T` is not the same as any of the remaining types,
    * and then recursively checks the rest.
    *
    * @tparam T The first type to compare.
    * @tparam Rest The remaining types in the parameter pack.
    */
    template <typename T, typename... Rest>
    struct is_unique<T, Rest...> : std::bool_constant<
    (!std::is_same_v<T, Rest> && ...) && is_unique<Rest...>::value
    > {};
    
    /**
    * @var is_unique_v
    * @brief Convenience variable template for `is_unique<Ts...>::value`.
    *
    * Evaluates to `true` if all types in the pack `Ts...` are distinct, `false` otherwise.
    *
    * @tparam Ts The types to check for uniqueness.
    *
    * @see is_unique
    */
    template <typename... Ts>
    inline constexpr bool is_unique_v = is_unique<Ts...>::value;
    
    
    /**
    * @brief Helper function to retrieve the underlying value of an enum.
    * 
    * This function template takes an enum value and returns its underlying type.
    * 
    * @tparam T an `enum class` type.
    * 
    * @param v the enum value to convert.
    * 
    */
    template<typename T>
    constexpr std::underlying_type_t<T> underlying_v(T v){
        return static_cast<std::underlying_type_t<T>>(v);
    }
    
    /**
    * @var always_false_v
    * @brief Template-dependent compile-time false value for triggering conditional static_assert.
    *
    * This utility is used in `if constexpr` or other SFINAE-based contexts to intentionally
    * trigger a `static_assert` only when a specific template branch is instantiated.
    * 
    * Unlike `false` or `std::false_type::value`, this variable is *dependent* on the template
    * parameter `T`, ensuring that the compiler will only evaluate it when that branch is chosen.
    * 
    * This is particularly useful in generic functions or traits where a catch-all `else` branch
    * should cause a compile-time error only if it is actually reached.
    *
    * @tparam T A template type used to make the expression type-dependent.
    *
    * @code
    * template <typename T>
    * void process(const T&) {
    *     if constexpr (std::is_integral_v<T>) {
    *         // Handle integers
    *     } else {
    *         static_assert(always_false_v<T>, "Unsupported type in process()");
    *     }
    * }
    * @endcode
    *
    * @note This is a common metaprogramming idiom adopted by many modern C++ codebases.
    */
    template <typename T>
    constexpr bool always_false_v = false;
    
    
    /**
    * @brief Provides a member typedef `type` that names `T`.
    *
    * This struct performs the identity transformation on a type `T`, effectively
    * returning the same type. It's particularly useful in template metaprogramming
    * to prevent type deduction in certain contexts.
    *
    * @tparam T The type to be encapsulated.
    */
    template <class T>
    struct type_identity {
        using type = T; /**< The encapsulated type. */
    };
    
    /**
    * @brief Helper alias template for `type_identity`.
    *
    * Provides a convenient way to access the encapsulated type without explicitly
    * specifying `::type`.
    *
    * @tparam T The type to be encapsulated.
    */
    template <class T>
    using type_identity_t = typename type_identity<T>::type;
} // namespace eser::tools

#endif // ESER_TRAITS_HPP_
