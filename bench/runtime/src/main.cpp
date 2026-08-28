/**
* @file main.cpp
*
* @brief Runtime throughput benchmark: nanoseconds per serialize/deserialize on real hardware.
*
* @ingroup eser_bench
*
* Prints a table over the serial port at 115200 baud. Flash it, open a monitor, and read the
* numbers; nothing needs to be attached beyond the board itself.
*
* A single flat serialize is far shorter than one tick of any available clock, so each case is run
* @ref iterations times inside a timed region and the total divided. The loop body folds its result
* into a `volatile` sink, without which the optimizer deletes the entire measurement.
*
* @note The reported cost includes the loop's own overhead (increment, compare, branch, and the
*       sink fold). An empty-loop calibration is measured first and subtracted, so the printed
*       figure is the codec's marginal cost rather than the raw loop time.
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
#include "eser/eser.hpp"

using namespace eser::flat;
using eser::utils::endianness;
using eser::utils::fixed_string;

namespace {
    /// Repetitions per timed region; large enough that clock granularity is negligible.
    constexpr uint32_t iterations = 20000;

    /// Sink the optimizer cannot prove unused, anchoring every measured loop.
    volatile uint32_t sink = 0;

    uint8_t buffer[128];

    struct pose { float x, y, z; uint16_t frame; };

    /// Microseconds since boot, from the most precise counter the target offers.
    inline uint64_t now_us()
    {
    #if defined(ESP32)
        return static_cast<uint64_t>(esp_timer_get_time());
    #else
        return static_cast<uint64_t>(micros());
    #endif
    }

    /// Nanoseconds per iteration for a timed region, with loop overhead already removed.
    double per_iteration_ns(uint64_t elapsed_us, uint64_t overhead_us)
    {
        const double net_us = (elapsed_us > overhead_us)
                            ? static_cast<double>(elapsed_us - overhead_us)
                            : 0.0;
        return (net_us * 1000.0) / static_cast<double>(iterations);
    }

    void report(const char *label, uint64_t elapsed_us, uint64_t overhead_us, size_t bytes)
    {
        const double ns = per_iteration_ns(elapsed_us, overhead_us);

        char line[96];
        // dtostrf is the portable way to format a double on cores whose printf lacks %f.
        char ns_text[16];
        dtostrf(ns, 0, 1, ns_text);

        snprintf(line, sizeof(line), "  %-34s %8s ns   %3u B", label, ns_text,
                 static_cast<unsigned>(bytes));
        Serial.println(line);
    }
} // namespace

void setup()
{
    Serial.begin(115200);
    while (!Serial) { /* wait for USB CDC on boards that need it */ }
    delay(300);

    Serial.println();
    Serial.println("=== eser runtime benchmark ===");
    Serial.print("iterations per case: ");
    Serial.println(iterations);
    Serial.println();

    // ---- loop-overhead calibration -------------------------------------------------
    // Mirrors the shape of every measured loop (counter, one buffer store, sink fold) so that
    // scaffolding is subtracted out and only the codec's own cost is reported.
    uint64_t t0 = now_us();
    for (uint32_t i = 0; i < iterations; ++i) {
        buffer[0] = static_cast<uint8_t>(i);
        sink += i;
    }
    const uint64_t overhead_us = now_us() - t0;

    Serial.print("loop overhead subtracted: ");
    Serial.print(static_cast<uint32_t>(overhead_us));
    Serial.println(" us total");
    Serial.println();
    Serial.println("  case                                   cost     wire");
    Serial.println("  ---------------------------------- --------   ------");

    size_t written = 0;

    // ---- serialize: scalars, native wire -------------------------------------------
    t0 = now_us();
    for (uint32_t i = 0; i < iterations; ++i) {
        written = serialize(i, static_cast<int16_t>(i), static_cast<float>(i)).to(buffer);
        sink += written;
    }
    report("serialize 3 scalars (native)", now_us() - t0, overhead_us, written);

    // ---- serialize: scalars, swapped wire ------------------------------------------
    t0 = now_us();
    for (uint32_t i = 0; i < iterations; ++i) {
        written = serialize<endianness::big>(i, static_cast<int16_t>(i), static_cast<float>(i)).to(buffer);
        sink += written;
    }
    report("serialize 3 scalars (big-endian)", now_us() - t0, overhead_us, written);

    // ---- deserialize: scalars, native wire -----------------------------------------
    // The first byte is rewritten every iteration. Reading a buffer that never changes lets the
    // compiler hoist the loads out of the loop entirely, which reports a deserialize far cheaper
    // than the equivalent serialize -- a measurement artifact, not a real asymmetry.
    serialize(uint32_t{7}, int16_t{-3}, 2.5f).to(buffer);
    t0 = now_us();
    for (uint32_t i = 0; i < iterations; ++i) {
        buffer[0] = static_cast<uint8_t>(i);
        auto got = deserialize(buffer, sizeof(buffer)).to<std::tuple<uint32_t, int16_t, float>>();
        // Every field must reach the sink. Folding only get<0> lets the compiler discard the
        // int16 and float reads as dead, timing a partial decode as if it were the whole one.
        if (got) sink += std::get<0>(*got)
                       + static_cast<uint32_t>(std::get<1>(*got))
                       + static_cast<uint32_t>(std::get<2>(*got));
    }
    report("deserialize 3 scalars (native)", now_us() - t0, overhead_us, 10);

    // ---- deserialize: scalars, swapped wire ----------------------------------------
    serialize<endianness::big>(uint32_t{7}, int16_t{-3}, 2.5f).to(buffer);
    t0 = now_us();
    for (uint32_t i = 0; i < iterations; ++i) {
        buffer[0] = static_cast<uint8_t>(i);
        auto got = deserialize<endianness::big>(buffer, sizeof(buffer))
                       .to<std::tuple<uint32_t, int16_t, float>>();
        if (got) sink += std::get<0>(*got)
                       + static_cast<uint32_t>(std::get<1>(*got))
                       + static_cast<uint32_t>(std::get<2>(*got));
    }
    report("deserialize 3 scalars (big-endian)", now_us() - t0, overhead_us, 10);

    // ---- array of 8 uint16 ----------------------------------------------------------
    {
        std::array<uint16_t, 8> arr{};
        t0 = now_us();
        for (uint32_t i = 0; i < iterations; ++i) {
            arr[0] = static_cast<uint16_t>(i);
            written = serialize(arr).to(buffer);
            sink += written;
        }
        report("serialize array<uint16,8>", now_us() - t0, overhead_us, written);
    }

    // ---- fixed_string ---------------------------------------------------------------
    {
        fixed_string<16> tag{"node-7"};
        t0 = now_us();
        for (uint32_t i = 0; i < iterations; ++i) {
            written = serialize(i, tag).to(buffer);
            sink += written;
        }
        report("serialize u32 + fixed_string<16>", now_us() - t0, overhead_us, written);
    }

    // ---- struct ---------------------------------------------------------------------
    {
        pose p{1.0f, 2.0f, 3.0f, 0};
        t0 = now_us();
        for (uint32_t i = 0; i < iterations; ++i) {
            p.frame = static_cast<uint16_t>(i);
            written = serialize(p).to(buffer);
            sink += written;
        }
        report("serialize struct pose (raw image)", now_us() - t0, overhead_us, written);
    }

    Serial.println();
    Serial.print("checksum (ignore): ");
    Serial.println(static_cast<uint32_t>(sink));
    Serial.println("=== done ===");
}

void loop() {}
