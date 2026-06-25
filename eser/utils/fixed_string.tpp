/**
* @file fixed_string.tpp
*
* @brief Definition of functionality in fixed_string.hpp
*
* @author Mark Tikhonov <mtik.philosopher@gmail.com>
*
* @date 2026-06-24
*
* @copyright
* MIT License
* SPDX-License-Identifier: MIT
*
* @par Changelog
* - 2026-06-24
* -     Initial creation.
*/
#ifndef ESER_UTILS_FIXED_STRING_TPP_
#define ESER_UTILS_FIXED_STRING_TPP_
#include "fixed_string.hpp"
#include <cassert>

namespace eser::utils{
    template<std::size_t N>
    constexpr fixed_string<N>::fixed_string() noexcept : _data{} {}

    template<std::size_t N>
    constexpr fixed_string<N>::fixed_string(std::string_view source) noexcept : _data{}
    {
        assert(source.size() <= N and "fixed_string: source longer than capacity N; it is truncated");
        const std::size_t count = source.size() < N ? source.size() : N;
        for (std::size_t i = 0; i < count; ++i) _data[i] = source[i];
    }

    template<std::size_t N>
    constexpr std::string_view fixed_string<N>::view() const noexcept
    {
        std::size_t length = 0;
        while (length < N and _data[length] != '\0') ++length;
        return std::string_view(_data, length);
    }

    template<std::size_t N>
    constexpr const char* fixed_string<N>::data() const noexcept
    {
        return _data;
    }

    template<std::size_t N>
    constexpr std::size_t fixed_string<N>::size() const noexcept { return view().size(); }

    template<std::size_t N>
    constexpr std::size_t fixed_string<N>::capacity() noexcept { return N; }

    template<std::size_t N>
    constexpr bool fixed_string<N>::operator==(const fixed_string& other) const noexcept
    {
        return view() == other.view();
    }

    template<std::size_t N>
    constexpr bool fixed_string<N>::operator!=(const fixed_string& other) const noexcept
    {
        return not (*this == other);
    }
} // namespace eser::utils

#endif // ESER_UTILS_FIXED_STRING_TPP_
