# eser – Serialization for Embedded & Systems C++

**eser** is a lightweight, serialization framework for modern C++17 environments, targeting embedded systems, real-time applications, and efficient protocol development.

It focuses on:
- **Portability**: Works across embedded platforms (e.g., ESP32) and desktop environments.
- **Efficiency**: No dynamic memory allocation or RTTI.
- **Safety**: Strongly typed.
- **Clarity**: Minimal, clean API using modern C++ idioms.


## Binary Serialization
---

- Works with:
  - Scalar types (e.g., `int`, `float`)
  - C-style arrays (e.g., `int[4]`)
  - `std::array`
  - Trivially copyable structs and enums
  - `eser::tools::fixed_string<N>` for fixed-capacity strings
- Wire byte order is a compile-time policy (**little-endian** by default; see Endianness below)




### Serialization

```cpp
#include <eser/flat/serializer.hpp>

using namespace eser::flat;

int id = 42;
float temperature = 36.6f;

auto s = serialize(id, temperature);

std::byte buffer[64]{};
size_t bytes_written = s.to(buffer);  // Writes bytes into buffer

// You can also serialize into a uint8_t array if required:

std::uint8_t raw[64]{};
s.to(raw);
```
### Deserialization

```cpp
#include <array>
#include "eser/flat/deserializer.hpp"

using namespace eser::flat;

std::byte buffer[64]{ /* previously filled with data */ };

// Construct deserializer from a byte buffer
auto d = deserialize(buffer);

// `to<T>()` returns std::optional<T>: nullopt when the buffer is too short.
// Read several fields by naming a std::tuple.
if (auto fields = d.to<std::tuple<int, float>>()) {
    auto& [id, temperature] = *fields;
    // ... use id, temperature
}

// For single values or arrays:

std::optional<int> value = d.to<int>();

auto array = d.to<std::array<int, 4>>(); // Returns std::optional<std::array<int, 4>>
```

### Example

```cpp
struct Message {
    int id;
    float value;
};

static_assert(std::is_trivially_copyable_v<Message>);

Message m{1, 2.5f};
auto s = serialize(m);

std::byte buffer[64];
s.to(buffer);

auto d = deserialize(buffer);
std::optional<Message> result = d.to<Message>();
```

### Strings

Strings are stored as a fixed-capacity `fixed_string<N>` field — `N` bytes on the wire, so the
format stays compile-time sized. Shorter strings are null-padded; reading back yields a
`std::string_view` (via `.view()`) that stops at the first null.

The capacity `N` is part of the contract, not derived from the string. Pick it when the field
has a fixed width independent of the content (e.g. a 16-byte name field), so both ends agree:

```cpp
#include "eser/tools/fixed_string.hpp"

using eser::tools::fixed_string;

std::byte buffer[64];
serialize(fixed_string<16>{"sensor"}).to(buffer); // always 16 bytes, whatever the string

auto name = deserialize(buffer).to<fixed_string<16>>(); // std::optional<fixed_string<16>>
if (name) {
    std::string_view sv = name->view(); // "sensor"
}
```

A bare string literal also serializes directly, but the field is then exactly the literal's
size (including its null), so the reader must use the matching width:

```cpp
serialize("sensor").to(buffer);                  // 7-byte field ("sensor" + '\0')
auto s = deserialize(buffer).to<fixed_string<7>>();
```

### 🧠 Use Cases
- Embedded task parameter passing

- Inter-MCU binary communication


- Framing & packing for custom protocols

### 📌 Endianness

The wire byte order is a compile-time policy, defaulting to little-endian:

```cpp
serialize(a, b).to(buffer);                          // little-endian (default)
serialize<endianness::big>(a, b).to(buffer);         // big-endian (network order)
auto x = deserialize<endianness::big>(buffer).to<...>();
```

When the wire order differs from the host, scalar fields (and arrays of scalars) are byte-reversed
via `if constexpr` — zero cost when they match. Trivially-copyable structs are serialized as raw
bytes and therefore cannot be byte-swapped: they may only be used when the wire order matches the
host (enforced by `static_assert`); split a struct into scalar fields if you need a non-native wire.

### ✅ Buffer Safety

The serializer does not allocate.
You must provide a sufficiently large buffer.
Use assertions or utilities to determine required size (planned feature).

--- 
## 🧪 Testing

Run
```bash
tools/run_tests.sh
```
## 🔐 License

MIT License

Copyright (c) 2025 Mark Tikhonov

Permission is hereby granted, free of charge, to any person obtaining a copy...

See LICENSE for full text.
📅 Changelog Highlights

    2025-07-02: Initial implementation of serializer/deserializer core.

    2025-08-05: License changed from CC BY-ND 4.0 to MIT. Library renamed from ser to eser.


## 🙋‍♂️ Contributing

    Fork → Create feature branch → PR

    Respect embedded constraints: no heap, no STL containers unless explicitly supported

    For larger contributions (custom formats, protocols), please open an issue first

## 📫 Contact

Feel free to open an issue or reach out via email **mtik.philosopher@gmail.com** or via GitHub for questions or collaboration proposals.