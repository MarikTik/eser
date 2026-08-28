# eser benchmarks

Four independent measurements. Only the last needs hardware attached.

| What | Command | Hardware |
|---|---|---|
| Static footprint, per feature, per build mode | `python3 scripts/measure.py` | no |
| Compile time | included in the above (`build` column) | no |
| Codegen quality (abstraction cost) | `scripts/codegen.sh [host\|xtensa\|arm]` | no |
| Runtime cost per call | `cd runtime && pio run -e esp32dev -t upload` | **yes** |

---

## Static footprint

```bash
python3 scripts/measure.py                          # every board, every mode
python3 scripts/measure.py --boards esp32dev        # one board
python3 scripts/measure.py --modes rel              # shipping config only
python3 scripts/measure.py --json out.json          # machine-readable records
```

Builds run **one at a time**, deliberately. Concurrent embedded builds each hold a large working
set; a handful in parallel is enough to trigger the OOM killer on an ordinary workstation, and the
compile-time column would then measure contention rather than the code. Once the frameworks are
cached a full environment takes about four seconds, so the sequential run is not the bottleneck.

Builds `<board>_<mode>_t<tier>` for each combination, reads true section sizes from each
firmware ELF, and prints absolute footprint plus the incremental cost of each feature tier.

**Feature tiers** (all compiled from the same `src/main.cpp`, selected by `-D BENCH_TIER=n`):

| Tier | Adds |
|---|---|
| 0 | framework + sink only, eser not included — the floor |
| 1 | `#include "eser/eser.hpp"`, nothing instantiated |
| 2 | scalar codec, native little-endian wire |
| 3 | array codec (C array and `std::array`) |
| 4 | big-endian wire (byte-reversal path) |
| 5 | `fixed_string` (endianness-neutral path) |
| 6 | trivially-copyable struct (raw image) |

**Build modes:**

| Mode | Flags | Notes |
|---|---|---|
| `rel` | `-Os -DNDEBUG` | what ships; asserts compiled out |
| `relO2` | `-O2 -DNDEBUG` | speed-optimized |
| `dbg` | `-Og` | eser's `assert` contracts are **live** and cost real bytes |

`platformio.ini` is generated — edit `scripts/gen_ini.py` and regenerate:

```bash
python3 scripts/gen_ini.py > platformio.ini
```

The driver verifies that the intended `-O` level and `NDEBUG` actually reached the compile line,
and prints a loud warning if not. PlatformIO applies `build_unflags` after `build_flags`, so an
over-broad unflag list silently strips the optimization level and leaves the build at `-O0` —
which inflates every number by roughly an order of magnitude without erroring.

### Results

Whole codec, tier 0 → 6:

| Board | rel | relO2 | dbg |
|---|---|---|---|
| esp32dev (ESP32-D0WD, LX6) | 1024 B | 1024 B | 2540 B |
| esp32s3 (ESP32-S3, LX7) | 1044 B | 1044 B | 2568 B |
| nodemcuv2 (ESP8266) | 888 B | 888 B | 3044 B |
| stm32f411 (Cortex-M4) | 564 B | 564 B | 3364 B |
| nano (AVR) | — | — | — (cannot build: no libstdc++) |

RAM cost is +128 B on every board and mode, which is the benchmark's own `uint8_t buffer[128]`,
not codec overhead — the codec itself allocates nothing.

Including the header without instantiating anything costs **exactly 0 bytes** in every
configuration measured.

---

## Codegen quality

```bash
scripts/codegen.sh host      # x86-64
scripts/codegen.sh xtensa    # ESP32/ESP8266
scripts/codegen.sh arm       # Cortex-M (compile-only; no board required)
```

Compiles a hand-written `memcpy` of three fields alongside the eser equivalent at native and
swapped wire order, then counts instructions in each. This is the one claim a size table cannot
settle: size deltas show the byte-swap path costs bytes, but only disassembly shows the *native*
path costs nothing.

| Target | hand-written | eser native | eser swapped |
|---|---|---|---|
| x86-64 `-O2` | 7 | **7** | 21 |
| Xtensa LX6 `-O2` | 24 | 27 | 31 |

On x86-64 the native path is instruction-for-instruction identical to hand-written `memcpy` — the
templates, the tuple, and the `std::apply` fold all compile away entirely.

On Xtensa it is 3 instructions longer. That gap is eser's runtime buffer-capacity check (`needed >
size`), which the bare `memcpy` reference does not perform; it is a real check, not overhead the
optimizer failed to remove. The zero-cost result is specific to x86-64 — on Xtensa the accurate
statement is "within 3 instructions of hand-written, and the difference is a bounds check."

---

## Runtime cost

Needs a board attached. Flashes a sketch that times each codec path and prints a table over
serial at 115200 baud.

```bash
cd runtime
pio run -e esp32dev -t upload
pio device monitor -b 115200
```

Environments: `esp32dev` (ESP32 / WROOM-32 / WROOM-32U) and `esp32s3` (ESP32-S3-WROOM-1).

### Measured — `-O2`, 20000 iterations/case, on hardware

| Case | ESP32 @240MHz | ESP32-S3 @240MHz | ESP8266 @80MHz | Wire |
|---|---|---|---|---|
| serialize 3 scalars (native) | 274.5 ns | 235.7 ns | 1139.1 ns | 10 B |
| serialize 3 scalars (big-endian) | 475.9 ns | 425.4 ns | 1538.8 ns | 10 B |
| deserialize 3 scalars (native) | 118.3 ns | 113.8 ns | 950.9 ns | 10 B |
| deserialize 3 scalars (big-endian) | 408.0 ns | 378.4 ns | 1538.5 ns | 10 B |
| serialize `array<uint16,8>` | 369.8 ns | 332.2 ns | 1200.4 ns | 16 B |
| serialize `u32 + fixed_string<16>` | 378.2 ns | 390.7 ns | 1000.4 ns | 20 B |
| serialize struct (raw image) | 181.1 ns | 189.4 ns | 587.9 ns | 16 B |

A non-native wire roughly doubles the cost of a scalar field set — the concrete price of the
byte-reversal path that `if constexpr` removes entirely when the orders match.

Each case subtracts a calibration loop of identical shape (counter, one buffer store, sink fold),
so the figure is the codec's marginal cost rather than loop scaffolding. Inputs vary per iteration
and every decoded field is folded into a `volatile` sink; without both, the optimizer hoists or
deletes the work and reports implausibly low numbers.

---

## Board coverage

Distinct boards are not distinct measurement targets:

- **ESP32 devkit, WROOM-32, WROOM-32U** all map to `esp32dev` — same ESP32-D0WD die and LX6 core.
  WROOM-32U differs only in using a U.FL external-antenna connector instead of a PCB trace
  antenna, which changes nothing measured here. They produce byte-identical firmware.
- **ESP32-S3-WROOM-1** is a separate target (Xtensa LX7) with genuinely different timings.
- **NodeMCU v2 and v3** both map to `nodemcuv2`; PlatformIO has no separate v3 id, as v3 boards
  use the same ESP-12E module.

---

## Requirements

PlatformIO CLI, plus `intelhex` in PlatformIO's own interpreter (ESP32 builds fail without it):

```bash
$(head -1 $(which pio) | sed 's/^#!//') -m pip install intelhex
```

The first ESP32 build downloads the Arduino-ESP32 framework and takes several minutes; later
builds take about five seconds.
