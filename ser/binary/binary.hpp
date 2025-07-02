/**
* @file binary.hpp
* @brief Aggregator header for the binary serialization and deserialization utilities.
*
* @defgroup ser_binary Binary Serialization Module
* @ingroup ser
*
* @brief Provides high-performance binary serialization and deserialization for modern C++.
*
* The `ser::binary` module includes:
* 
* - @ref ser::binary::serializer "serializer" - Converts C++ objects and arrays into a raw byte stream.
* - @ref ser::binary::deserializer "deserializer" - Reconstructs C++ objects and arrays from a byte stream.
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
* #include "ser/binary/binary.hpp"
*
* using namespace ser::binary;
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
* @note The deserializer invalidates itself as it consumes bytes from the input stream.
*/

#ifndef SER_BINARY_BINARY_HPP_
#define SER_BINARY_BINARY_HPP_
#include "serializer.hpp"
#include "deserializer.hpp"
#endif // SER_BINARY_BINARY_HPP_