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
  - Trivially copyable structs
  - Enums and null-terminated C-strings
- For now, Assumes **little-endian** byte order




### Serialization

```cpp
#include <eser/binary/serializer.hpp>

using namespace eser::binary;

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
#include "eser/binary/deserializer.hpp"

using namespace eser::binary;

std::byte buffer[64]{ /* previously filled with data */ };

// Construct deserializer from a byte buffer
auto d = deserialize(buffer);

// Extract into typed values
auto [id, temperature] = d.to<int, float>();

// For single values or arrays:

int value = d.to<int>();

auto array = d.to<int[4]>(); // Returns std::array<int, 4>
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
Message result = d.to<Message>();
```
### 🧠 Use Cases
- Embedded task parameter passing

- Inter-MCU binary communication


- Framing & packing for custom protocols

### 📌 Endianness

Currently, binary serialization is little-endian only. On big-endian systems, convert data before/after.

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