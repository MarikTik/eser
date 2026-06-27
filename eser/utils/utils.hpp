/**
* @file utils.hpp
*
* @brief Aggregator header for all utilities in `eser/utils/`.
*
* @defgroup eser_utils eser::utils
*
* @ingroup eser
*
* The `eser_utils` group is the public utility surface:
* - The byte-order policy (`endianness.hpp`: the `endianness` enum and `is_endianness_neutral`)
* - A fixed-capacity string value type (`fixed_string.hpp`)
*
* (Internal machinery — the requirements guard, type traits, and byte-swapping helpers — lives in
* `eser/internal/` and is not part of the public API.)
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
* - 2026-06-24
*       Internal machinery (`byte.hpp`, `traits.hpp`, endianness host-detection/byte-swapping)
*       moved to `eser/internal/`; `utils/` now holds only the public surface.
*/
#ifndef ESER_UTILS_UTILS_HPP_
#define ESER_UTILS_UTILS_HPP_
#include "endianness.hpp"
#include "fixed_string.hpp"
#endif // ESER_UTILS_UTILS_HPP_
