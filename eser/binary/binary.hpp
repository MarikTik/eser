/**
* @file binary.hpp
*
* @brief Aggregator header for the binary serialization and deserialization utilities.
*
* @defgroup eser_binary eser::binary
*
* @ingroup eser
*
* @brief Provides high-performance binary serialization and deserialization for modern C++.
*
* The `eser::binary` module includes:
* 
* - @ref eser::binary::serializer "serializer" - Converts C++ objects and arrays into a raw byte stream.
* - @ref eser::binary::deserializer "deserializer" - Reconstructs C++ objects and arrays from a byte stream.
*
* This module is designed for:
* 
* - Embedded systems requiring efficient binary data processing.
* - Applications needing compact storage formats.
* - Systems where direct memory representation of data types is desirable for performance.
*
* ## Supported Types
*
* The binary module currently supports serialization and deserialization of:
*
* - Trivially copyable scalars (integers, floats, enums)
* - C-style arrays
* - `std::array`
* - `std::string_view` (for serialization) # not maintained, prefer fixed-size arrays
* - Trivially copyable structs
*
* ## Endianness
*
* All serialization and deserialization in this module assumes **little-endian** encoding.
*
* ## Usage Example
*
* ```cpp
* #include "eser/binary/binary.hpp"
*
* using namespace eser::binary;
*
* int a = 42;
* float b = 3.14f;
*
* auto s = serialize(a, b);
*
* std::uint8_t buffer[64]{};
* size_t bytes_written = s.to(buffer);
*
* auto d = deserialize(buffer, bytes_written);
* auto [x, y] = d.to<int, float>();
*
* assert(x == 42);
* assert(y == 3.14f);
* ```
*
* @note The serialization and deserialization classes are not thread-safe
* when used concurrently on the same instance.
*
* @note The deserializer invalidates itself as it consumes bytes from the input stream.
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
#ifndef ESER_BINARY_BINARY_HPP_
#define ESER_BINARY_BINARY_HPP_
#include "serializer.hpp"
#include "deserializer.hpp"
#endif // ESER_BINARY_BINARY_HPP_