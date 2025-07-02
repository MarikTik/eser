/**
* @file ser.hpp
* @brief Master include header for the `ser` serialization library.
*
* @defgroup ser Serialization Library
* @brief A modern, header-only C++17 library for efficient data serialization and deserialization.
*
* The `ser` library provides a lightweight framework for serializing and deserializing data
* in embedded systems, applications, and any project requiring:
* 
* - Compact binary representation
* - Simple API surface
* - Header-only deployment
* - C++17 support for modern templates and constexpr functionality
*
* ## Submodules
*
* - @ref ser_tools "ser_tools" : Utility components and type traits used across serialization tasks.
* - @ref ser_binary "ser_binary" : High-performance binary serialization and deserialization utilities.
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
* #include "ser/ser.hpp"
*
* using namespace ser::binary;
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
* @author Mark Tikhonov 
* @date Jul 2 2025
*/
#ifndef SER_HPP_
#define SER_HPP_
#include "tools/tools.hpp"
#include "binary/binary.hpp"
#endif // SER_HPP_
