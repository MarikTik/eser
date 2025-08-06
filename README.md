# eser – Lightweight C++ Serialization for Embedded and Systems Programming

`eser` is a collection of minimalistic, efficient C++ serializers and deserializers designed for embedded systems such as the ESP32, but equally usable in desktop/server environments. The project provides fast and type-safe facilities for converting objects into raw byte streams and back, with a focus on:

- Portability (desktop + embedded)
- Efficiency (zero dynamic allocation in core APIs)
- Extensibility (easily plug in custom types or serialization strategies)
- Clarity (simple, modern C++)

## Current Modules

- `eser::binary`: A binary serializer/deserializer targeting trivially copyable types and arrays, useful for task parameter encoding, raw data communication, and internal buffers.

## Project Goals

- Keep things lightweight, fast, and deterministic
- Support CI and test-driven extension
- Provide specialized serializers over time (e.g., for text, compact encoding, etc.)

## Getting Started

1. Clone the repository
2. Include the desired module headers (e.g. `serializer.hpp`, `deserializer.hpp`)
3. Use `eser::binary::serialize(...)` and `seer::binary::deserialize(...)` to encode/decode data.

## Contributing

At this stage, the repository is maintained by the original author. If you'd like to contribute:
- Fork the repo and open a PR with a clear summary of the feature or bug fix.
- Please respect the minimalistic and embedded-friendly goals of the project.
- Extensive modifications or new serializers should be proposed via issue first.

Direct edits or redistribution of modified versions is **not allowed** by default. This is to maintain a consistent direction and stability across embedded deployments. If your use case requires deeper changes, please open an issue or proposal first.

## License

This project is licensed under the MIT License. You are free to use, modify, and distribute the code for both commercial and non-commercial purposes, provided you include the original copyright notice and the license text.

For the full license text, please see the [LICENSE](LICENSE) file.

## Roadmap

- Add constexpr-deserialization support
- Add CI tests with common C++ platforms (ESP-IDF, Linux GCC, Clang, MSVC)
- Extend test coverage and benchmarks
- Explore optional runtime-safe deserialization variants

---

For questions, issues, or suggestions, open an issue on GitHub or contact the maintainer.
