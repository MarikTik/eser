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
* - @ref eser_binary "eser_binary" : High-performance binary serialization and deserialization utilities.
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
* using namespace eser::binary;
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
* auto [x, y] = d.to<int, float>();
*
* assert(x == 42);
* assert(y == 3.14f);
* ```
*
* @note All serialization in the binary module assumes little-endian encoding.
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
#include "binary/binary.hpp"
#endif // ESER_HPP_
