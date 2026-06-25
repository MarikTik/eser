/**
* @file tools.hpp
*
* @brief Aggregator header for all utilities in `ser/tools/`.
*
* @defgroup eser_tools eser::tools
*
* @ingroup eser
*
* The `eser_tools` group includes:
* - A toolchain requirements guard (`byte.hpp`: C++17 + `std::byte`)
* - The byte-order policy (`endianness.hpp`)
* - Type traits for metaprogramming
* - Miscellaneous utilities supporting serialization and embedded development
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
#ifndef ESER_TOOLS_TOOLS_HPP_
#define ESER_TOOLS_TOOLS_HPP_
#include "byte.hpp"
#include "utils.hpp"
#include "traits.hpp"
#include "endianness.hpp"
#include "fixed_string.hpp"
#endif // ESER_TOOLS_TOOLS_HPP_