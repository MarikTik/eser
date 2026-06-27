/**
* @file traits.hpp
*
* @brief Internal type traits used by the codec.
*
* @ingroup eser_internal
*
* @warning Implementation detail. Do not include directly or depend on `eser::internal`; it is not
*          part of the public API and may change between releases.
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
*       License changed from CC BY-ND 4.0 to MIT. Library renamed from `ser` to `eser`.
* - 2026-06-24
* -     Moved to `eser::internal` (eser/internal/traits.hpp). Removed dead `is_unique` and
*       `underlying_v`; `is_endianness_neutral` (a public customization point) moved to
*       `eser/utils/endianness.hpp`.
*/
#ifndef ESER_INTERNAL_TRAITS_HPP_
#define ESER_INTERNAL_TRAITS_HPP_

#include <type_traits>
#include <tuple>       // For the std::tuple specialization of is_tuple
#include <array>       // For the std::array specialization of is_std_array
#include <cstddef>     // For std::size_t

namespace eser::internal {
    /**
    * @struct is_tuple
    * @brief Detects whether a type is a `std::tuple` specialization.
    *
    * @tparam T The type to inspect.
    * @see is_tuple_v
    */
    template <typename T>
    struct is_tuple : std::false_type {};

    /**
    * @brief Specialization of `is_tuple` matching any `std::tuple` instantiation.
    * @tparam Ts The element types of the tuple.
    */
    template <typename... Ts>
    struct is_tuple<std::tuple<Ts...>> : std::true_type {};

    /**
    * @var is_tuple_v
    * @brief Convenience variable template for `is_tuple<T>::value`.
    * @tparam T The type to inspect.
    */
    template <typename T>
    inline constexpr bool is_tuple_v = is_tuple<T>::value;


    /**
    * @struct is_std_array
    * @brief Detects whether a type is a `std::array` specialization.
    *
    * @tparam T The type to inspect.
    * @see is_std_array_v
    */
    template <typename T>
    struct is_std_array : std::false_type {};

    /**
    * @brief Specialization of `is_std_array` matching any `std::array` instantiation.
    * @tparam T The element type of the array.
    * @tparam N The number of elements in the array.
    */
    template <typename T, std::size_t N>
    struct is_std_array<std::array<T, N>> : std::true_type {};

    /**
    * @var is_std_array_v
    * @brief Convenience variable template for `is_std_array<T>::value`.
    * @tparam T The type to inspect.
    */
    template <typename T>
    inline constexpr bool is_std_array_v = is_std_array<T>::value;


    /**
    * @var always_false_v
    * @brief Template-dependent compile-time `false` for triggering a conditional `static_assert`.
    *
    * Unlike `false`, this variable is *dependent* on `T`, so the compiler only evaluates it when the
    * enclosing branch is actually instantiated — the idiom for a catch-all `else` that should fail
    * to compile only if reached.
    *
    * @tparam T A template type used to make the expression type-dependent.
    */
    template <typename T>
    constexpr bool always_false_v = false;


    /**
    * @struct type_identity
    * @brief Names `T` via a member typedef; blocks template argument deduction.
    *
    * @tparam T The type to encapsulate.
    */
    template <class T>
    struct type_identity {
        using type = T; /**< The encapsulated type. */
    };

    /**
    * @brief Helper alias for `type_identity<T>::type`.
    * @tparam T The type to encapsulate.
    */
    template <class T>
    using type_identity_t = typename type_identity<T>::type;
} // namespace eser::internal

#endif // ESER_INTERNAL_TRAITS_HPP_
