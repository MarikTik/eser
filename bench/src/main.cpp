/**
* @file main.cpp
*
* @brief Static-footprint benchmark sketch: one codec feature tier per build.
*
* @ingroup eser_bench
*
* The tier is selected by `-D BENCH_TIER=<n>` from platformio.ini. Every tier compiles from this
* same translation unit, so the difference between two consecutive tiers is the incremental flash
* and RAM cost of exactly the codec paths that tier adds, and nothing else.
*
* Tier 0 is the floor: framework plus the sink, with no eser header included at all. Because eser is
* header-only, tier 1 (include only) shows what the headers cost when nothing is instantiated —
* expected to be zero, and worth measuring precisely because it is a claim the README makes.
*
* @note `volatile` sink and `consume()` exist to defeat dead-code elimination. Without them the
*       linker discards every serialize call and all tiers collapse to the tier-0 size.
*
* @author Mark Tikhonov <mtik.philosopher@gmail.com>
*
* @date 2026-08-27
*
* @copyright
* MIT License
* SPDX-License-Identifier: MIT
*/
#include <Arduino.h>
#include <stdint.h>

#ifndef BENCH_TIER
#  error "[eser bench] BENCH_TIER is undefined; build via bench/platformio.ini which sets -D BENCH_TIER=<n>"
#endif

#if BENCH_TIER < 0 || BENCH_TIER > 6
#  error "[eser bench] BENCH_TIER must be in [0, 6]; see bench/platformio.ini for the tier table"
#endif

#if BENCH_TIER >= 1
#  include "eser/eser.hpp"
#endif

/// Sink that the optimizer cannot prove unused, anchoring every tier's generated code.
volatile uint32_t bench_sink = 0;

/// Fold a written byte range into @ref bench_sink so the serialize call cannot be elided.
static void consume(const uint8_t *data, size_t written)
{
    uint32_t acc = static_cast<uint32_t>(written);
    for (size_t i = 0; i < written; ++i) acc = (acc * 31u) + data[i];
    bench_sink = bench_sink + acc;
}

#if BENCH_TIER >= 2
using namespace eser::flat;
using eser::utils::endianness;
#endif

#if BENCH_TIER >= 5
using eser::utils::fixed_string;
#endif

#if BENCH_TIER >= 6
/// Trivially-copyable aggregate exercising the raw-image (native-endian only) path.
struct pose { float x, y, z; uint16_t frame; };
#endif

static uint8_t buffer[128];

void setup()
{
    // Values are read through the volatile sink so none of them is a compile-time constant;
    // otherwise the codec would fold away and the tier would measure nothing.
    const uint32_t seed = bench_sink;

#if BENCH_TIER >= 2
    // Tier 2: scalar path (integers + float), native little-endian wire.
    {
        const uint32_t a = seed;
        const int16_t  b = static_cast<int16_t>(seed);
        const float    c = static_cast<float>(seed) * 0.5f;
        consume(buffer, serialize(a, b, c).to(buffer));

        auto got = deserialize(buffer, sizeof(buffer)).to<std::tuple<uint32_t, int16_t, float>>();
        if (got) bench_sink += std::get<0>(*got);
    }
#endif

#if BENCH_TIER >= 3
    // Tier 3: array path — element-wise conversion over C arrays and std::array.
    {
        uint16_t raw[4] = {
            static_cast<uint16_t>(seed),      static_cast<uint16_t>(seed + 1),
            static_cast<uint16_t>(seed + 2),  static_cast<uint16_t>(seed + 3)
        };
        consume(buffer, serialize(raw).to(buffer));

        auto got = deserialize(buffer, sizeof(buffer)).to<std::array<uint16_t, 4>>();
        if (got) bench_sink += (*got)[3];
    }
#endif

#if BENCH_TIER >= 4
    // Tier 4: big-endian wire — the byte-reversal path that tier 2 compiles out entirely.
    {
        const uint32_t a = seed;
        const int16_t  b = static_cast<int16_t>(seed);
        const float    c = static_cast<float>(seed) * 0.5f;
        consume(buffer, serialize<endianness::big>(a, b, c).to(buffer));

        auto got = deserialize<endianness::big>(buffer, sizeof(buffer))
                       .to<std::tuple<uint32_t, int16_t, float>>();
        if (got) bench_sink += std::get<0>(*got);
    }
#endif

#if BENCH_TIER >= 5
    // Tier 5: fixed_string — the endianness-neutral pass-through path.
    {
        fixed_string<16> tag{"node-7"};
        consume(buffer, serialize(seed, tag).to(buffer));

        auto got = deserialize(buffer, sizeof(buffer)).to<std::tuple<uint32_t, fixed_string<16>>>();
        if (got) bench_sink += std::get<1>(*got).size();
    }
#endif

#if BENCH_TIER >= 6
    // Tier 6: trivially-copyable struct — raw memcpy image, native endianness only.
    {
        pose p{static_cast<float>(seed), 2.0f, 3.0f, static_cast<uint16_t>(seed)};
        consume(buffer, serialize(p).to(buffer));

        auto got = deserialize(buffer, sizeof(buffer)).to<pose>();
        if (got) bench_sink += got->frame;
    }
#endif

    (void)seed;
}

void loop() {}
