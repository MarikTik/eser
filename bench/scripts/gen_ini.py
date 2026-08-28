#!/usr/bin/env python3
"""Generate bench/platformio.ini: the cross product of boards, feature tiers, and build modes.

Hand-maintaining tiers x modes x boards env blocks invites copy-paste drift, and a benchmark whose
environments quietly disagree measures nothing. Regenerate instead::

    python3 bench/scripts/gen_ini.py > bench/platformio.ini

Build modes matter as much as tiers here: eser's second-tier error handling is ``assert``, which
exists only while ``NDEBUG`` is undefined. A release-only table would report the contract checks as
free because they were compiled out.
"""

from __future__ import annotations

import sys

# (board id, platformio board, comment) -- only targets that are actually measured.
BOARDS = [
    ("esp32dev", "esp32dev", "espressif32",
     "ESP32-D0WD (LX6). Plain ESP32 devkit, WROOM-32 and WROOM-32U all map here:\n"
     "; same die and core; WROOM-32U differs only by U.FL external antenna."),
    ("esp32s3", "esp32-s3-devkitc-1", "espressif32",
     "ESP32-S3-WROOM-1 (LX7). Distinct core from the LX6 above."),
    ("nodemcuv2", "nodemcuv2", "espressif8266",
     "ESP8266 NodeMCU. PlatformIO has no separate v3 id; v3 boards use this same ESP-12E target."),
    ("stm32f411", "blackpill_f411ce", "ststm32",
     "STM32F411 (Cortex-M4). Compile-only portability check: no such board is on hand, so its\n"
     "; footprint is measured but its runtime cost is not."),
    ("nano", "nanoatmega328", "atmelavr",
     "Arduino Nano (ATmega328P, AVR). Compile-only compatibility probe: avr-gcc ships a reduced\n"
     "; libstdc++ (std::optional may be absent) and its double is 32-bit rather than IEEE-754\n"
     "; binary64, either of which can make the codec fail to compile. Failure here is a result,\n"
     "; not a broken benchmark."),
]

# (mode id, build_type, extra flags, extra unflags) -- how the same source is compiled.
#
# The debug mode must unflag -DNDEBUG: some platforms (STM32duino among them) inject it into
# their own default build_flags even for build_type = debug, which silently compiles out every
# assert and makes the debug column measure the same thing as release.
MODES = [
    ("rel", "release", "-Os -DNDEBUG", ""),
    ("relO2", "release", "-O2 -DNDEBUG", ""),
    ("dbg", "debug", "-Og", "-DNDEBUG"),
]

TIERS = range(0, 7)

TIER_COMMENT = """; Tier table (see bench/src/main.cpp):
;   0  framework + sink only, eser not included   -> the floor
;   1  + #include "eser/eser.hpp", nothing used   -> header-only cost of inclusion
;   2  + scalar codec, native little-endian wire
;   3  + array codec (C array and std::array)
;   4  + big-endian wire (byte-reversal path)
;   5  + fixed_string (endianness-neutral path)
;   6  + trivially-copyable struct (raw image)
;
; Build modes:
;   rel    -Os -DNDEBUG   size-optimized release; asserts compiled out (what ships)
;   relO2  -O2 -DNDEBUG   speed-optimized release; asserts compiled out
;   dbg    -Og            debug; eser's assert-based contracts are LIVE and cost real bytes"""


def main() -> int:
    out: list[str] = []
    out.append("; eser static-footprint benchmark -- GENERATED FILE, DO NOT EDIT BY HAND.")
    out.append("; Regenerate with: python3 bench/scripts/gen_ini.py > bench/platformio.ini")
    out.append(";")
    out.append("; Each [env:<board>_<mode>_t<n>] builds bench/src/main.cpp at one feature tier and")
    out.append("; one build mode. Subtracting consecutive tiers within a mode gives the incremental")
    out.append("; cost of the codec paths that tier adds.")
    out.append(";")
    out.append(TIER_COMMENT)
    out.append("")
    out.append("[platformio]")
    out.append("src_dir = src")
    out.append("")
    out.append("[env]")
    out.append("framework = arduino")
    # Only the language standard is unflagged. Listing -Os/-O2/-Og here would strip the
    # optimization level this benchmark is trying to set: build_unflags is applied after
    # build_flags, so the mode's own -O would be removed and every tier would silently
    # compile at -O0, inflating the measured cost by an order of magnitude.
    out.append("build_unflags = -std=gnu++11 -std=gnu++14")
    out.append("lib_deps =")
    out.append("")

    for board_id, pio_board, platform, comment in BOARDS:
        out.append(f"; {'-' * 74}")
        for line in comment.split("\n"):
            out.append(f"; {line}" if not line.startswith(";") else line)
        out.append(f"[{board_id}_base]")
        out.append(f"platform = {platform}")
        out.append(f"board = {pio_board}")
        out.append("")

        for mode_id, build_type, mode_flags, mode_unflags in MODES:
            for tier in TIERS:
                out.append(f"[env:{board_id}_{mode_id}_t{tier}]")
                out.append(f"extends = {board_id}_base")
                out.append(f"build_type = {build_type}")
                out.append(
                    f"build_flags = -std=gnu++17 {mode_flags} "
                    f"-I${{PROJECT_DIR}}/.. -D BENCH_TIER={tier}"
                )
                if mode_unflags:
                    out.append(f"build_unflags = -std=gnu++11 -std=gnu++14 {mode_unflags}")
                out.append("")

    print("\n".join(out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
