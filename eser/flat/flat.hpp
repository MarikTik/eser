/**
* @file flat.hpp
*
* @brief Aggregator header for the binary serialization and deserialization utilities.
*
* @defgroup eser_flat eser::flat
*
* @ingroup eser
*
* @brief Provides high-performance binary serialization and deserialization for modern C++.
*
* The `eser::flat` module includes:
* 
* - @ref eser::flat::serializer "serializer" - Converts C++ objects and arrays into a raw byte stream.
* - @ref eser::flat::deserializer "deserializer" - Reconstructs C++ objects and arrays from a byte stream.
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
* - `eser::utils::fixed_string<N>` — a fixed-capacity string field (read via `to<fixed_string<N>>()`)
* - Trivially copyable structs
*
* ## Endianness
*
* The wire byte order is a compile-time policy (`eser::utils::endianness`), defaulting to
* little-endian. Pass it explicitly — `serialize<endianness::big>(...)`,
* `deserialize<endianness::big>(...)` — to read/write the other order; scalars are byte-reversed
* via `if constexpr` only when the wire order differs from the host. Raw structs are restricted
* to the native order (they cannot be byte-swapped).
*
* ## Usage Example
*
* ```cpp
* #include "eser/flat/flat.hpp"
*
* using namespace eser::flat;
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
* auto fields = d.to<std::tuple<int, float>>(); // std::optional<std::tuple<int, float>>
* assert(fields);
* auto& [x, y] = *fields;
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
#ifndef ESER_FLAT_BINARY_HPP_
#define ESER_FLAT_BINARY_HPP_
#include "serializer.hpp"
#include "deserializer.hpp"
#include "size.hpp"
#endif // ESER_FLAT_BINARY_HPP_