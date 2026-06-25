/**
* @file utils.hpp
*
* @brief Aggregator header for all utilities in `eser/utils/`.
*
* @defgroup eser_utils eser::utils
*
* @ingroup eser
*
* The `eser_utils` group includes:
* - A toolchain requirements guard (`byte.hpp`: C++17 + `std::byte`)
* - The byte-order policy (`endianness.hpp`)
* - Type traits for metaprogramming (`traits.hpp`)
* - A fixed-capacity string value type (`fixed_string.hpp`)
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
* - 2026-06-24
*       Directory `eser/tools` renamed to `eser/utils`; `serialized_size_of` moved out to
*       `eser/flat/serialized_size_of.hpp` (it is specific to the flat format).
*/
#ifndef ESER_UTILS_UTILS_HPP_
#define ESER_UTILS_UTILS_HPP_
#include "byte.hpp"
#include "traits.hpp"
#include "endianness.hpp"
#include "fixed_string.hpp"
#endif // ESER_UTILS_UTILS_HPP_
