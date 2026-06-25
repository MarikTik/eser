/**
* @file eser.hpp
*
* @brief Master include header for the `eser` serialization library.
*
* @defgroup eser Serialization Library
*
* @brief A modern, header-only C++17 library for efficient data serialization and deserialization.
*
* The `eser` library provides a lightweight framework for serializing and deserializing data
* in embedded systems, applications, and any project requiring:
* 
* - Compact binary representation
* - Simple API surface
* - Header-only deployment
* - C++17 support for modern templates and constexpr functionality
*
* ## Submodules
*
* - @ref eser_tools "eser_tools" : Utility components and type traits used across serialization tasks.
* - @ref eser_flat "eser_flat" : High-performance binary serialization and deserialization utilities.
*
* ## Features
*
* - Serialization of scalar types, arrays, and trivially copyable structs
* - Type traits and helpers for modern metaprogramming
* - Support for embedded systems with constrained resources
* - Portable and standard-compliant C++17 codebase
*
* ## Usage Example
*
* ```cpp
* #include "eser/eser.hpp"
*
* using namespace eser::flat;
*
* int a = 42;
* float b = 3.14f;
*
* auto s = serialize(a, b);
*
* std::uint8_t buffer[64]{};
* size_t written = s.to(buffer);
*
* auto d = deserialize(buffer, written);
* auto fields = d.to<std::tuple<int, float>>(); // std::optional<std::tuple<int, float>>
* assert(fields);
* auto& [x, y] = *fields;
*
* assert(x == 42);
* assert(y == 3.14f);
* ```
*
* @note Wire byte order is a compile-time policy (`eser::tools::endianness`), little-endian by default.
*
* @author Mark Tikhonov <mtik.philosopher@gmail.com>
*
* @date Jul 2 2025
*
* @copyright
* Creative Commons Attribution-NoDerivatives 4.0 International Public License
* See https://creativecommons.org/licenses/by-nd/4.0/
* SPDX-License-Identifier: CC-BY-ND-4.0
*/
#ifndef ESER_HPP_
#define ESER_HPP_
#include "tools/tools.hpp"
#include "flat/flat.hpp"
#endif // ESER_HPP_
