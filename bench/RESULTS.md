# eser benchmark results

Raw measured data. Every figure below was produced by the harness in this directory; nothing is
estimated or carried over from another machine.

Host: Linux, PlatformIO CLI. Library version 1.1.1 (commit `16e89e9`).
Measured 2026-08-27.

Each section states whether it is **COMPILE-TIME** (headless, no board) or **RUNTIME** (measured on
attached hardware), and which board produced it.

---

## 1. RUNTIME — ESP32-D0WD (regular ESP32, WROOM-32), attached via /dev/ttyUSB0

Measured on hardware. 240 MHz, `-O2`, 20000 iterations per case, loop overhead subtracted.

| Case | Cost | Wire bytes |
|---|---|---|
| serialize 3 scalars (native LE) | 274.5 ns | 10 |
| serialize 3 scalars (big-endian) | 475.9 ns | 10 |
| deserialize 3 scalars (native LE) | 118.3 ns | 10 |
| deserialize 3 scalars (big-endian) | 408.0 ns | 10 |
| serialize `array<uint16,8>` | 369.8 ns | 16 |
| serialize `u32 + fixed_string<16>` | 378.2 ns | 20 |
| serialize struct `pose` (raw image) | 181.1 ns | 16 |

Calibration loop overhead: 929 us total across 20000 iterations.

Notes:
- A non-native wire roughly doubles scalar cost (274.5 -> 475.9 ns serialize; 118.3 -> 408.0 ns
  deserialize). That is the concrete price of the byte-reversal path.
- Deserialize is cheaper than serialize because it performs no buffer-capacity check and its reads
  hit cached flash.
- Two earlier revisions of this table reported deserialize at 33.5 ns and then 45.8 ns. Both were
  measurement artifacts (constant input buffer; partially-dead decoded fields) and were corrected
  before the numbers above were taken. Both fixes *raised* the reported cost. See
  `runtime/src/main.cpp` comments.

---

## 2. COMPILE-TIME — static footprint, ESP32-D0WD (`esp32dev`)

Headless. No board involved.

### rel (-Os -DNDEBUG) — the shipping configuration

| Tier | flash | dflash | ram | dram | build |
|---|---|---|---|---|---|
| 0 framework only (no eser) | 233198 | — | 21048 | — | 5.5 s |
| 1 + include eser.hpp | 233198 | **+0** | 21048 | +0 | 5.1 s |
| 2 + scalar codec (native LE) | 233406 | +208 | 21176 | +128 | 4.9 s |
| 3 + array codec | 233506 | +100 | 21176 | +0 | 4.8 s |
| 4 + big-endian wire | 233842 | +336 | 21176 | +0 | 5.1 s |
| 5 + fixed_string | 234106 | +264 | 21176 | +0 | 5.1 s |
| 6 + struct (raw image) | 234222 | +116 | 21176 | +0 | 4.9 s |

**Total: +1024 B flash, +128 B RAM**

### relO2 (-O2 -DNDEBUG)

Identical tier-by-tier to `rel`; total **+1024 B flash, +128 B RAM**.

The two modes producing identical sizes was verified as genuine, not a flag error: `cmp` shows the
`rel` and `relO2` firmware ELFs differ, and the flag checker confirmed `-Os` and `-O2` each reached
the compile line. The sizes coincide because eser's contribution is dominated by inlined `memcpy`
stores that both levels handle the same way at this scale.

### dbg (-Og, asserts live)

| Tier | flash | dflash | ram | dram | build |
|---|---|---|---|---|---|
| 0 framework only (no eser) | 233198 | — | 21048 | — | 5.2 s |
| 1 + include eser.hpp | 233198 | **+0** | 21048 | +0 | 5.2 s |
| 2 + scalar codec (native LE) | 233618 | +420 | 21176 | +128 | 5.5 s |
| 3 + array codec | 234430 | +812 | 21176 | +0 | 5.3 s |
| 4 + big-endian wire | 235046 | +616 | 21176 | +0 | 5.1 s |
| 5 + fixed_string | 235450 | +404 | 21176 | +0 | 5.3 s |
| 6 + struct (raw image) | 235738 | +288 | 21176 | +0 | 4.4 s |

**Total: +2540 B flash, +128 B RAM**

---

## 3. COMPILE-TIME — static footprint, ESP8266 NodeMCU (`nodemcuv2`)

Headless. No board involved. (Also covers NodeMCU v3 — same ESP-12E target.)

### rel (-Os -DNDEBUG)

| Tier | flash | dflash | ram | dram | build |
|---|---|---|---|---|---|
| 0 framework only (no eser) | 260211 | — | 27144 | — | 5.2 s |
| 1 + include eser.hpp | 260211 | **+0** | 27144 | +0 | 4.6 s |
| 2 + scalar codec (native LE) | 260451 | +240 | 27272 | +128 | 4.7 s |
| 3 + array codec | 260547 | +96 | 27272 | +0 | 4.7 s |
| 4 + big-endian wire | 260899 | +352 | 27272 | +0 | 4.9 s |
| 5 + fixed_string | 261051 | +152 | 27264 | -8 | 4.8 s |
| 6 + struct (raw image) | 261099 | +48 | 27264 | +0 | 4.7 s |

**Total: +888 B flash, +120 B RAM**

### relO2 (-O2 -DNDEBUG)

Identical to `rel`; total **+888 B flash, +120 B RAM**.

### dbg (-Og, asserts live)

| Tier | flash | dflash | ram | dram | build |
|---|---|---|---|---|---|
| 0 framework only (no eser) | 262505 | — | 27152 | — | 5.2 s |
| 1 + include eser.hpp | 262505 | **+0** | 27152 | +0 | 5.3 s |
| 2 + scalar codec (native LE) | 263033 | +528 | 27280 | +128 | 5.5 s |
| 3 + array codec | 263893 | +860 | 27280 | +0 | 5.4 s |
| 4 + big-endian wire | 264661 | +768 | 27280 | +0 | 5.4 s |
| 5 + fixed_string | 265197 | +536 | 27272 | -8 | 5.5 s |
| 6 + struct (raw image) | 265549 | +352 | 27272 | +0 | 5.6 s |

**Total: +3044 B flash, +120 B RAM**

### RUNTIME — ESP8266EX (NodeMCU v3), attached via /dev/ttyUSB0

Measured on hardware. 80 MHz, `-O2`, 20000 iterations per case, loop overhead subtracted
(2505 us total).

| Case | ESP8266 | ESP32 (LX6) | ratio |
|---|---|---|---|
| serialize 3 scalars (native LE) | 1139.1 ns | 274.5 ns | 4.2x |
| serialize 3 scalars (big-endian) | 1538.8 ns | 475.9 ns | 3.2x |
| deserialize 3 scalars (native LE) | 950.9 ns | 118.3 ns | 8.0x |
| deserialize 3 scalars (big-endian) | 1538.5 ns | 408.0 ns | 3.8x |
| serialize `array<uint16,8>` | 1200.4 ns | 369.8 ns | 3.2x |
| serialize `u32 + fixed_string<16>` | 1000.4 ns | 378.2 ns | 2.6x |
| serialize struct `pose` (raw image) | 587.9 ns | 181.1 ns | 3.2x |

Roughly 3-4x slower than the ESP32 across the board, which tracks the clock difference (80 MHz
single-core LX106 vs 240 MHz LX6) plus the ESP8266's slower flash-cached instruction fetch.

Two things worth noting:
- The byte-swap penalty is *proportionally smaller* here (1139 -> 1539 ns, +35%) than on the ESP32
  (274 -> 476 ns, +73%). On the slower core the fixed `memcpy` and call overhead dominate, so the
  extra reversal work is a smaller share of the total.
- Deserialize shows the largest gap (8.0x). Chip identity was confirmed by `esptool chip_id`
  (ESP8266EX, CH340 bridge, VID:PID `1A86:7523`) before flashing.

---

## 4. COMPILE-TIME — static footprint, ESP32-S3-WROOM-1 (`esp32s3`)

Headless. No board involved.

### rel (-Os -DNDEBUG)

| Tier | flash | dflash | ram | dram | build |
|---|---|---|---|---|---|
| 0 framework only (no eser) | 364114 | — | 18056 | — | 4.4 s |
| 1 + include eser.hpp | 364114 | **+0** | 18056 | +0 | 4.4 s |
| 2 + scalar codec (native LE) | 364322 | +208 | 18184 | +128 | 4.3 s |
| 3 + array codec | 364422 | +100 | 18184 | +0 | 4.4 s |
| 4 + big-endian wire | 364758 | +336 | 18184 | +0 | 4.7 s |
| 5 + fixed_string | 365038 | +280 | 18184 | +0 | 4.3 s |
| 6 + struct (raw image) | 365158 | +120 | 18184 | +0 | 4.4 s |

**Total: +1044 B flash, +128 B RAM**

### relO2 (-O2 -DNDEBUG)

Identical to `rel`; total **+1044 B flash, +128 B RAM**.

### dbg (-Og, asserts live)

| Tier | flash | dflash | ram | dram | build |
|---|---|---|---|---|---|
| 0 framework only (no eser) | 364114 | — | 18056 | — | 4.5 s |
| 1 + include eser.hpp | 364114 | **+0** | 18056 | +0 | 4.5 s |
| 2 + scalar codec (native LE) | 364534 | +420 | 18184 | +128 | 4.8 s |
| 3 + array codec | 365334 | +800 | 18184 | +0 | 4.5 s |
| 4 + big-endian wire | 365970 | +636 | 18184 | +0 | 4.7 s |
| 5 + fixed_string | 366374 | +404 | 18184 | +0 | 4.6 s |
| 6 + struct (raw image) | 366682 | +308 | 18184 | +0 | 4.5 s |

**Total: +2568 B flash, +128 B RAM**

The LX7 tracks the LX6 closely (1044 vs 1024 B rel), which is expected: same ISA family, same
codegen strategy. The larger tier-0 floor (364 KB vs 233 KB) is the S3's bigger Arduino framework,
not eser.

### RUNTIME — ESP32-S3 (QFN56 rev v0.2, 8 MB PSRAM), attached via /dev/ttyUSB0

Measured on hardware. 240 MHz, `-O2`, 20000 iterations per case, loop overhead subtracted
(752 us total).

| Case | S3 (LX7) | ESP32 (LX6) | S3 vs LX6 |
|---|---|---|---|
| serialize 3 scalars (native LE) | 235.7 ns | 274.5 ns | -14% |
| serialize 3 scalars (big-endian) | 425.4 ns | 475.9 ns | -11% |
| deserialize 3 scalars (native LE) | 113.8 ns | 118.3 ns | -4% |
| deserialize 3 scalars (big-endian) | 378.4 ns | 408.0 ns | -7% |
| serialize `array<uint16,8>` | 332.2 ns | 369.8 ns | -10% |
| serialize `u32 + fixed_string<16>` | 390.7 ns | 378.2 ns | +3% |
| serialize struct `pose` (raw image) | 189.4 ns | 181.1 ns | +5% |

The LX7 is 4-14% faster on every scalar and array path at the same 240 MHz clock. The two cases
where it is marginally slower (`fixed_string`, struct) are both dominated by a bulk `memcpy` rather
than per-field work, where the core generation matters less than memory behaviour.

Both boards were identified by `esptool chip_id` before flashing rather than inferred from the USB
descriptor: this S3 board uses a CP2102 UART bridge (VID:PID `10C4:EA60`), the same descriptor a
classic ESP32 devkit presents, so the USB ID alone does not distinguish them.

---

## 4b. COMPILE-TIME — static footprint, STM32F411 (Cortex-M4)

Headless. No such board is on hand, so this is **footprint only** — runtime cost is not measured.

**Result: eser builds cleanly on Cortex-M, and is cheapest there of any target.**

### rel (-Os -DNDEBUG)

| Tier | flash | dflash | ram | dram | build |
|---|---|---|---|---|---|
| 0 framework only (no eser) | 7768 | — | 1096 | — | 3.5 s |
| 1 + include eser.hpp | 7768 | **+0** | 1096 | +0 | 3.2 s |
| 2 + scalar codec (native LE) | 7828 | +60 | 1224 | +128 | 3.3 s |
| 3 + array codec | 7912 | +84 | 1224 | +0 | 3.3 s |
| 4 + big-endian wire | 8080 | +168 | 1224 | +0 | 3.5 s |
| 5 + fixed_string | 8224 | +144 | 1224 | +0 | 3.8 s |
| 6 + struct (raw image) | 8332 | +108 | 1224 | +0 | 3.3 s |

**Total: +564 B flash, +128 B RAM**

### relO2 (-O2 -DNDEBUG)

Identical to `rel`; total **+564 B flash, +128 B RAM**.

### dbg (-Og, asserts live)

| Tier | flash | dflash | ram | dram | build |
|---|---|---|---|---|---|
| 0 framework only (no eser) | 16712 | — | 1252 | — | 3.1 s |
| 1 + include eser.hpp | 16712 | **+0** | 1252 | +0 | 3.2 s |
| 2 + scalar codec (native LE) | 17088 | +376 | 1380 | +128 | 3.2 s |
| 3 + array codec | 18260 | +1172 | 1380 | +0 | 3.2 s |
| 4 + big-endian wire | 18844 | +584 | 1380 | +0 | 3.1 s |
| 5 + fixed_string | 19576 | +732 | 1380 | +0 | 3.4 s |
| 6 + struct (raw image) | 20076 | +500 | 1380 | +0 | 3.2 s |

**Total: +3364 B flash, +128 B RAM**

At 564 B release, Cortex-M is the cheapest target measured — roughly half the Xtensa cost — which
is consistent with Thumb-2 being a denser encoding than Xtensa for this kind of load/store code.
Its framework floor is also far smaller (7.8 KB vs 233-364 KB on the ESP platforms).

> **Correction applied.** The first `dbg` run here reported 1944 B. That was wrong: the STM32duino
> platform injects `-DNDEBUG` into its own default flags even for `build_type = debug`, silently
> compiling out every assert, so the "debug" column was measuring a release build. The harness's
> flag checker caught it (`-DNDEBUG present in a debug build; asserts are compiled out`), and the
> generator now unflags `-DNDEBUG` in debug mode. The corrected figure is 3364 B. The ESP platforms
> were re-verified and were never affected — their numbers are unchanged.

First build took 26 minutes, almost entirely fetching the STM32 framework; subsequent builds are
about three seconds.

---

## 5. COMPILE-TIME — Arduino Nano / AVR compatibility probe

Headless. No board involved. **Result: eser cannot be built for AVR.**

The limitation is the toolchain's standard library, not the language standard and not eser:

| Check | Result |
|---|---|
| `avr-g++` accepts `-std=gnu++17` | **yes** — `if constexpr` compiles |
| `<type_traits>` | MISSING |
| `<optional>` | MISSING |
| `<tuple>` | MISSING |
| `<array>` | MISSING |
| `<cstddef>`, `<cstring>`, `<limits>`, `<cstdint>` | MISSING |

avr-gcc ships **no libstdc++ headers at all**. The first failure is:

```
eser/utils/endianness.hpp:32:10: fatal error: type_traits: No such file or directory
```

Tier 0 (framework only, no eser) builds fine at 460 B flash / 13 B RAM, confirming the AVR
toolchain itself works — the failure is specifically the absence of the C++ standard library.

So the C++17 *language* is available on AVR (as the `-std=gnu++17` flag has been since GCC 5), but
the C++ *library* is not. eser depends on `std::optional`, `std::tuple`, `std::array` and
`std::is_trivially_copyable`, none of which exist there. Supporting AVR would mean vendoring
replacements for all of them — a different project, not a build flag.

Third-party shims (ArduinoSTL, avr-stl) provide parts of the library, but none supplies a
C++17-conformant `std::optional`, so none closes this gap.

---

## 6. COMPILE-TIME — codegen quality

Headless. Instruction counts from `objdump -d`, comparing eser against a hand-written `memcpy` of
the same three fields.

| Target | hand-written | eser native | eser swapped |
|---|---|---|---|
| x86-64, `-O2` | 7 | **7** | 21 |
| Xtensa LX6, `-O2` | 24 | 27 | 31 |

On x86-64 the native-endian path is instruction-for-instruction identical to hand-written
`memcpy`: the templates, tuple, and `std::apply` fold compile away completely.

On Xtensa the native path is 3 instructions longer. That gap is eser's runtime buffer-capacity
check (`needed > size`), which the bare `memcpy` reference does not perform — a real check, not
un-eliminated overhead. The accurate cross-architecture statement is "within 3 instructions of
hand-written, and the difference is a bounds check", not an unqualified zero-cost claim.

The swapped-wire column confirms byte-reversal appears only when the wire order differs from the
host, on both architectures.

---

## 7. Cross-cutting observations

1. **Including the header costs exactly 0 bytes** on every board and every build mode measured.
2. **RAM cost is 0.** The +128 B at tier 2 is the benchmark's own `uint8_t buffer[128]`, carried by
   all later tiers. The codec allocates nothing.
3. **Asserts are the largest single cost factor**, roughly 2.5x (esp32dev) to 3.4x (nodemcuv2) the
   shipping footprint. They vanish under `NDEBUG`.
4. **The big-endian path is the most expensive feature tier** in flash on both boards, and roughly
   doubles runtime cost — consistent with it being the only tier that adds real per-field work
   rather than another `memcpy` shape.

---

## Reproducing

```bash
cd bench
python3 scripts/measure.py                 # COMPILE-TIME, all boards/modes
scripts/codegen.sh host                    # COMPILE-TIME, codegen
scripts/codegen.sh xtensa

cd runtime                                 # RUNTIME, needs a board attached
pio run -e esp32dev -t upload
pio device monitor -b 115200
```
