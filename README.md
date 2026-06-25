# eser — Flat Binary Serialization for Embedded & Systems C++

**eser** is a lightweight, header-only C++17 serialization library for turning trivially-copyable
values into compact, fixed-layout byte streams and back. It is built for embedded targets (ESP32 and
similar), real-time systems, and custom binary protocols where predictability and zero overhead
matter more than runtime flexibility.

The format is **flat and tagless**: fields are written back-to-back with no type tags, length
prefixes, or version markers. Both ends agree on the layout at compile time; the wire carries only
the data. In exchange you get no dynamic allocation, no exceptions, no RTTI, and a wire size that is
known at compile time.

```cpp
#include "eser/eser.hpp"
using namespace eser::flat;

std::uint32_t id = 42;
float temperature = 36.6f;

std::byte buffer[64];
std::size_t written = serialize(id, temperature).to(buffer);   // 8 bytes

if (auto fields = deserialize(buffer).to<std::tuple<std::uint32_t, float>>()) {
    auto& [r_id, r_temp] = *fields;                            // 42, 36.6f
}
```

---

## Table of Contents

- [Overview](#overview)
- [Requirements](#requirements)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Core Concepts](#core-concepts)
- [Supported Types](#supported-types)
- [Serialization](#serialization)
- [Deserialization](#deserialization)
- [Strings (`fixed_string`)](#strings-fixed_string)
- [Endianness](#endianness)
- [Buffer Sizing](#buffer-sizing)
- [Edge Cases & Behavior](#edge-cases--behavior)
- [Assumptions & Limitations](#assumptions--limitations)
- [When to Use eser (and When Not To)](#when-to-use-eser-and-when-not-to)
- [Thread Safety](#thread-safety)
- [Testing](#testing)
- [Project Layout](#project-layout)
- [License](#license)
- [Contributing](#contributing)
- [Contact](#contact)

---

## Overview

- **Header-only.** Drop it in or pull it via CMake `FetchContent`; nothing to build.
- **No allocation, no exceptions, no RTTI.** Suitable for `-fno-exceptions` / `-fno-rtti` embedded builds.
- **Compile-time sized.** Every supported type has a fixed wire size; buffers can be sized at compile time.
- **Strongly typed, optional-based reads.** `to<T>()` returns `std::optional<T>` and never reads past the buffer.
- **Compile-time endianness policy.** Little-endian by default; opt into big-endian (network order) per call at zero runtime cost.

Two namespaces are used throughout:

| Namespace | Contents |
|---|---|
| `eser::flat` | `serialize`, `deserialize`, `serializer`, `deserializer`, `serialized_size_of`, and the `endianness` alias |
| `eser::utils` | `fixed_string`, `endianness`, and supporting type traits |

---

## Requirements

- **C++17** or newer, with a conforming standard library that provides `std::byte`
  (`std::optional`, `std::string_view`, `std::tuple`, `std::array`).
- A single requirements guard (`eser/utils/byte.hpp`) fails the build with one clear message if the
  toolchain is pre-C++17 or lacks `std::byte`.

---

## Installation

eser is header-only. Either add the repository root to your include path and `#include "eser/eser.hpp"`,
or fetch it with CMake:

```cmake
include(FetchContent)
FetchContent_Declare(
    eser
    GIT_REPOSITORY https://github.com/MarikTik/eser.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(eser)

target_link_libraries(my_app PRIVATE eser)   # `eser` is an INTERFACE target
```

Include the umbrella header for everything, or pull only what you use:

```cpp
#include "eser/eser.hpp"                  // everything (eser::flat + eser::utils)
// or, selectively:
#include "eser/flat/serializer.hpp"
#include "eser/flat/deserializer.hpp"
#include "eser/utils/fixed_string.hpp"
```

---

## Quick Start

```cpp
#include "eser/eser.hpp"
#include <cstdint>

using namespace eser::flat;

struct sample {
    std::uint16_t sensor_id;
    float value;
};
static_assert(std::is_trivially_copyable_v<sample>);

void produce(std::byte* out, std::size_t out_size) {
    sample s{7, 23.5f};
    std::size_t written = serialize(s).to(out, out_size);   // 0 if the buffer is too small
    (void)written;
}

void consume(const std::byte* in, std::size_t in_size) {
    if (auto s = deserialize(in, in_size).to<sample>()) {
        // *s is a valid `sample`
    } else {
        // buffer too short — nothing was read
    }
}
```

---

## Core Concepts

**Flat & tagless format.** Values are serialized one after another with nothing between them — no
field IDs, no lengths, no version byte. The reader reconstructs values purely from the *types you
name*, in the *order you name them*. There is no self-description: the sender and receiver must
share the exact field layout (see [Assumptions & Limitations](#assumptions--limitations)).

**Two-phase API, used as one expression.**

- `serialize(args...)` builds a lightweight `serializer` that **captures its arguments by
  forwarding reference** (lvalues by reference — zero copy — rvalues by value); `.to(buffer)`
  writes them and returns the number of bytes written.
- `deserialize(data, length)` builds a `deserializer` — a **non-owning, consuming cursor** over the
  buffer; each `to<T>()` reads one value and advances the cursor.

**The serializer is a temporary — use it immediately.** Because it holds references to your
arguments, its `to()` overloads are **rvalue-ref-qualified** (`&&`): you must write
`serialize(...).to(buffer)` as a single expression. Storing it is a compile error, by design, so it
can never outlive the variables it references:

```cpp
serialize(a, b).to(buffer);          // OK — the only intended form

auto s = serialize(a, b);            // compiles (a reference-holding temporary materialized)...
s.to(buffer);                        // ...but THIS does not compile: `s` is an lvalue
```

This keeps capture zero-copy (no duplication of your data) while making the dangling/aliasing
scenario unrepresentable.

**Optional, all-or-nothing reads.** `to<T>()` returns `std::optional<T>`. If the buffer holds fewer
bytes than `T` requires, it returns `std::nullopt` and reads nothing — there is no partial fill.

---

## Supported Types

| Category | Examples | Notes |
|---|---|---|
| Scalars | `int`, `std::uint32_t`, `float`, `double`, `char`, `bool` | `bool` is normalized on read (see edge cases) |
| Enums | `enum class cmd : std::uint8_t { ... }` | stored as the underlying integer |
| `std::array` | `std::array<int, 4>`, nested arrays | serialized element-by-element |
| C-arrays *(serialize input only)* | `int[4]`, `"literal"` | serialized element-wise (same wire bytes as `std::array`); read back as `std::array` |
| Trivially-copyable structs / PODs | `struct vec3 { float x, y, z; };` | raw `memcpy` incl. padding; native-endian only (unless neutral) |
| `eser::utils::fixed_string<N>` | `fixed_string<16>` | fixed-capacity string field; endianness-neutral |

Every serialized type must satisfy `std::is_trivially_copyable_v<T>`. Types containing pointers,
references, virtuals, or heap-owning members are **not** supported (and unsafe to send — see
[Assumptions & Limitations](#assumptions--limitations)).

---

## Serialization

`serialize<Wire = endianness::little>(args...)` returns a `serializer` that captures `args` by
forwarding reference. Call one of its `.to(...)` overloads — in the **same expression** (they are
`&&`-qualified) — to write into a caller-provided buffer:

| Overload | Use |
|---|---|
| `to(std::byte* buffer, std::size_t size)` | pointer + explicit size |
| `to(std::byte (&buffer)[N])` | fixed-size `std::byte` array (size deduced) |
| `to(std::uint8_t* buffer, std::size_t size)` | legacy `uint8_t` buffer |
| `to(std::uint8_t (&buffer)[N])` | legacy fixed-size `uint8_t` array |

Each returns the number of bytes written (`std::size_t`), or `0` if the buffer is too small (see
[Edge Cases](#edge-cases--behavior)).

```cpp
using namespace eser::flat;
std::byte buffer[128];

// Scalars and multiple fields
serialize(std::uint32_t{1000}, 3.14f, true).to(buffer);

// A single value
serialize(std::int64_t{-42}).to(buffer);

// Arrays — a C-array is accepted; its wire bytes match a std::array (read back as std::array)
std::int32_t xs[4] = {1, -2, 3, -4};
serialize(xs).to(buffer);

// std::array directly
serialize(std::array<std::uint16_t, 3>{10, 20, 30}).to(buffer);

// Enums (written as the underlying type)
enum class cmd : std::uint8_t { stop, go, turn };
serialize(cmd::go).to(buffer);

// Trivially-copyable structs
struct vec3 { float x, y, z; };
serialize(vec3{1.f, 2.f, 3.f}).to(buffer);

// Legacy uint8_t buffer
std::uint8_t raw[128];
std::size_t n = serialize(42).to(raw);             // or: .to(raw, sizeof(raw))
```

---

## Deserialization

`deserialize<Wire = endianness::little>(data, length)` (or an array overload) returns a
`deserializer`. Read values with `to<T>()`:

| Overload | Use |
|---|---|
| `deserialize(const std::byte* data, std::size_t length)` | pointer + length |
| `deserialize(const std::byte (&data)[N])` | fixed-size `std::byte` array |
| `deserialize(const std::uint8_t* data, std::size_t length)` | legacy `uint8_t` buffer |
| `deserialize(const std::uint8_t (&data)[N])` | legacy fixed-size `uint8_t` array |

**Single value** — name the type, get `std::optional<T>`:

```cpp
auto v = deserialize(buffer).to<std::uint32_t>();   // std::optional<std::uint32_t>
if (v) use(*v);
```

**Several fields** — name a `std::tuple`; this is how you read more than one value:

```cpp
auto fields = deserialize(buffer).to<std::tuple<std::uint32_t, float, bool>>();
if (fields) {
    auto& [id, value, flag] = *fields;
}
```

**Arrays and structs** — name the exact type you want back:

```cpp
auto arr = deserialize(buffer).to<std::array<std::int32_t, 4>>();   // optional<std::array<...>>
auto v   = deserialize(buffer).to<vec3>();                         // optional<vec3>
```

**Consuming cursor.** A `deserializer` advances as it reads, so sequential calls walk the buffer:

```cpp
auto d = deserialize(buffer, length);
auto a = d.to<std::uint32_t>();   // reads bytes [0, 4)
auto b = d.to<float>();           // reads bytes [4, 8)
// once the remaining bytes are insufficient, to<T>() returns std::nullopt
```

> You name the type you want. To read multiple fields in one call, wrap them in a `std::tuple`;
> `to<int, float>()` (variadic) is **not** part of the API.

---

## Strings (`fixed_string`)

Strings are modeled as a **fixed-capacity** field, `eser::utils::fixed_string<N>`, which occupies
exactly `N` bytes on the wire. This keeps the format compile-time sized. It is a trivially-copyable,
byte-only value, so it travels through the normal serialize/deserialize paths.

```cpp
#include "eser/utils/fixed_string.hpp"
using eser::utils::fixed_string;

std::byte buffer[64];
serialize(fixed_string<16>{"sensor"}).to(buffer);          // always 16 bytes

if (auto name = deserialize(buffer).to<fixed_string<16>>()) {
    std::string_view sv = name->view();                     // "sensor"
}
```

**The capacity `N` is part of the contract, not derived from the string.** Choose it for a field of
fixed width (e.g. a 16-byte name) so both ends agree. A shorter string is null-padded; a string that
exactly fills `N` carries no terminator.

`fixed_string<N>` API (all `constexpr`, `noexcept`):

| Member | Returns | Notes |
|---|---|---|
| `fixed_string()` | — | all-zero field |
| `fixed_string(std::string_view)` | — | copies up to `N` bytes; longer source is truncated |
| `view()` | `std::string_view` | bounded by the first `'\0'`, or all `N` if full; **O(N)** scan |
| `data()` | `const char*` | raw pointer; **not guaranteed null-terminated** |
| `size()` | `std::size_t` | logical length (up to first `'\0'`) |
| `capacity()` | `std::size_t` | `N` (static) |
| `operator==`, `operator!=` | `bool` | compare by logical (`view()`) contents |

A bare string literal also serializes directly, but then the field is exactly the literal's size
(including its `'\0'`), so the reader must use the matching width:

```cpp
serialize("sensor").to(buffer);                       // 7-byte field ("sensor" + '\0')
auto s = deserialize(buffer).to<fixed_string<7>>();
```

> **Warning:** `data()` is not a C string when the field is full. Passing it to `strlen`,
> `printf("%s", ...)`, etc. reads past `N` into adjacent memory (crash / info leak). Use `view()`.

---

## Endianness

The wire byte order is a **compile-time policy**, `eser::utils::endianness` (aliased as
`eser::flat::endianness`), defaulting to little-endian:

```cpp
using namespace eser::flat;   // brings `endianness` into scope

serialize(id, value).to(buffer);                       // little-endian (default)
serialize<endianness::big>(id, value).to(buffer);      // big-endian (network order)

auto x = deserialize<endianness::big>(buffer)
             .to<std::tuple<std::uint32_t, float>>();
```

How it works:

- When the wire order **matches** the host, conversion is a no-op — **zero runtime cost** (selected via `if constexpr`).
- When it **differs**, scalars (and the scalars inside arrays, recursively) are byte-reversed on the boundary.
- **Trivially-copyable structs are raw bytes** and cannot be byte-swapped; they may only be used when
  the wire order matches the host. Using a non-native wire with a struct is a `static_assert`. Split
  the struct into scalar fields, or mark a byte-only type as endianness-neutral (next point).
- **Endianness-neutral types** (`eser::utils::is_endianness_neutral`) pass through unchanged on any
  wire because their meaningful units are single bytes. `fixed_string<N>` is neutral, so strings work
  over a big-endian wire. Specialize the trait for your own byte-only value types.

```cpp
// Big-endian writes most-significant byte first:
serialize<endianness::big>(std::uint32_t{0x01020304}).to(buffer);
// buffer[0..3] == { 0x01, 0x02, 0x03, 0x04 }
```

> **Both ends must agree on the wire order.** There is no endianness marker on the wire; reading a
> big-endian stream as little-endian yields silently byte-swapped values.

---

## Buffer Sizing

`eser::flat::serialized_size_of<Ts...>()` returns, at compile time, the exact number of bytes the
given types occupy on the wire in the flat format. Use it to size buffers:

```cpp
#include "eser/eser.hpp"
using eser::flat::serialized_size_of;
using eser::utils::fixed_string;

constexpr std::size_t N =
    serialized_size_of<std::uint32_t, float, fixed_string<16>>();

std::byte buffer[N];
serialize(std::uint32_t{1}, 2.0f, fixed_string<16>{"x"}).to(buffer);   // fits exactly
```

The serializer also checks the buffer at runtime: if the destination is smaller than required it
refuses to write (see [Edge Cases](#edge-cases--behavior)), so it never overflows a correctly-sized
buffer.

---

## Edge Cases & Behavior

| Situation | Behavior |
|---|---|
| Deserialize buffer shorter than `T` | `to<T>()` returns `std::nullopt`; nothing is read. Applies to scalars, tuples, arrays, and structs — strictly all-or-nothing (no partial/zero fill). |
| Serialize into an undersized buffer | **Debug** (`assert` enabled): aborts with a diagnostic. **Release** (`NDEBUG`): `to(...)` returns `0` and writes nothing. Either way, no overflow. Check the return value. |
| `to<bool>()` reads a byte other than `0`/`1` | Normalized: any non-zero byte → `true`. Prevents a trap `bool` (UB) from untrusted bytes. Applies to standalone `to<bool>()` and `bool` **tuple elements**; a `bool` nested in a struct or `std::array` is copied wholesale and is **not** normalized. |
| `fixed_string` source longer than `N` | **Debug:** asserts. **Release:** truncated to `N`. |
| `fixed_string` content exactly fills `N` | No null terminator; `view()`/`size()` still correct (bounded by `N`). `data()` is not a C string. |
| Endianness of reader ≠ writer | Silently byte-swapped values (no wire marker). Both ends must agree. |
| Deserializer cursor exhausted | Subsequent `to<T>()` calls return `std::nullopt`. |
| `to<std::tuple<>>()` (empty tuple) | `static_assert` — name at least one field. |
| Floating-point type that is not IEEE-754 | `static_assert` — does not compile (the codec assumes `iec559` layout). |
| `null` data pointer to `deserialize(...)` | `assert` in debug builds only (caller precondition, not a wire condition). |

---

## Assumptions & Limitations

These are deliberate. eser optimizes for compact, predictable, same-endpoint binary exchange — read
this section before putting it on a wire you do not control on both sides.

1. **Trivially-copyable only.** Every type is copied with `memcpy`. Types with pointers, references,
   virtuals, or owned heap memory are unsupported — and unsafe to send (you would reconstruct raw
   pointers from wire bytes).
2. **Flat & tagless — no self-description.** There are no type tags, length prefixes, or version
   markers. The sender and receiver **must share the exact field schema (types and order)**. A schema
   mismatch produces silently corrupt data, not an error.
3. **Structs are raw bytes, including padding.** A struct goes on the wire as its in-memory image:
   padding bytes (indeterminate) are included, and the layout is compiler/ABI-dependent. Round-trips
   are reliable only between endpoints built with the **same ABI** (same compiler/arch/flags). Structs
   are also native-endian only (unless endianness-neutral). For portable layouts, serialize explicit
   scalar fields instead of a struct, or `static_assert(std::has_unique_object_representations_v<T>)`
   to reject padding.
4. **Floating-point is IEEE-754.** Enforced by `static_assert`; both ends must share the representation.
5. **Endianness is a contract.** Configurable (little by default), but there is no marker on the wire —
   both ends must agree.
6. **No semantic validation.** Deserialization reconstructs bytes; it validates *length* only, never
   semantic invariants (ranges, enum membership, checksums). Validation belongs to your protocol layer.
7. **`length` is trusted.** `deserialize(data, length)` bounds every read by `length`. Pass only the
   number of bytes you actually hold; in particular, validate any wire-supplied length (e.g. a length
   prefix) against the bytes received **before** passing it, or reads will run past the buffer.
8. **Buffer ownership is the caller's.** The serializer writes into your buffer; the deserializer is a
   non-owning cursor and must not outlive the buffer it reads.
9. **The serializer is a use-immediately temporary.** It captures arguments by reference and its
   `to()` is `&&`-qualified, so it must be written in one expression (`serialize(...).to(buffer)`)
   and cannot be stored — this is enforced at compile time.
10. **No allocation, no exceptions, no RTTI.** Errors surface as `std::optional` (reads) or a `0`
    return / debug `assert` (writes), never as throws.
11. **Single-threaded per instance.** See [Thread Safety](#thread-safety).

---

## When to Use eser (and When Not To)

**Good fit**

- Fixed-layout binary packets between endpoints you control on both sides (same codebase / ABI).
- Embedded and inter-MCU communication (ESP32 and similar), where allocation and exceptions are off.
- Compact storage / framing for custom protocols with a known, stable schema.

**Reach for something else when you need**

- A cross-language or cross-platform wire format → Protocol Buffers, FlatBuffers, Cap'n Proto.
- Parsing untrusted/public input with built-in validation → a schema-validating format above.
- Variable-length, self-describing, or version-evolving messages → a tagged/length-prefixed format.

---

## Thread Safety

A single `serializer` or `deserializer` instance is **not** thread-safe; do not call `.to(...)` on the
same instance, or write into the same buffer, from multiple threads concurrently. Separate instances
operating on separate buffers are independent and safe to use in parallel.

---

## Testing

Tests use Catch2 and live under `tests/flat/`. Configure and run:

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j
cd build && ctest --output-on-failure
# or:
tools/run_tests.sh
```

A few contract tests cover release-only behavior (an undersized serialize buffer returning `0`,
`fixed_string` truncation) and are guarded by `NDEBUG`; build with `-DNDEBUG` to exercise them.

---

## Project Layout

```
eser/
  eser.hpp                 # umbrella include (utils + flat)
  flat/
    flat.hpp               # aggregator for the codec
    serializer.hpp/.tpp    # serialize() / serializer<Wire, T...>
    deserializer.hpp/.tpp  # deserialize() / deserializer<Wire>
    size.hpp # compile-time wire size of the flat format
  utils/
    utils.hpp              # aggregator for utilities
    byte.hpp               # C++17 + std::byte requirements guard
    endianness.hpp         # endianness enum, host detection, conversion helpers
    fixed_string.hpp/.tpp  # fixed_string<N>
    traits.hpp             # type traits (is_tuple, is_std_array, is_endianness_neutral, ...)
tests/flat/                # Catch2 test suite
```

---

## License

MIT License — Copyright (c) 2025 Mark Tikhonov. See [LICENSE](LICENSE) for the full text.

**Changelog highlights**

- 2025-07-02 — Initial serializer/deserializer core.
- 2025-08-05 — License changed from CC BY-ND 4.0 to MIT; library renamed `ser` → `eser`.

---

## Contributing

- Fork → feature branch → PR.
- Respect the embedded constraints: no heap, no exceptions/RTTI, no STL containers beyond what is
  explicitly supported.
- For larger contributions (new formats, protocol layers), please open an issue first.

---

## Contact

Open an issue, or reach out via **mtik.philosopher@gmail.com** or GitHub for questions and
collaboration proposals.
