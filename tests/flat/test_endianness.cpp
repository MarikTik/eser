#include <catch2/catch_all.hpp>
#include <cstdint>
#include <tuple>
#include <array>
#include "eser/flat/serializer.hpp"
#include "eser/flat/deserializer.hpp"
#include "eser/utils/fixed_string.hpp"

using namespace eser::flat;
using eser::utils::endianness;
using eser::utils::fixed_string;

constexpr std::size_t EBUF = 64;
static std::uint8_t e_buffer[EBUF];

void e_clear() { std::fill(e_buffer, e_buffer + EBUF, 0); }

TEST_CASE("host_endianness matches a runtime probe") {
    std::uint16_t one = 1;
    const auto* bytes = reinterpret_cast<const unsigned char*>(&one);
    const bool host_is_little = (bytes[0] == 1);
    REQUIRE((eser::internal::host_endianness == endianness::little) == host_is_little);
}

TEST_CASE("big-endian serialization writes most-significant byte first") {
    e_clear();
    std::uint32_t v = 0x01020304u;
    auto written = serialize<endianness::big>(v).to(e_buffer);
    REQUIRE(written == 4);
    REQUIRE(e_buffer[0] == 0x01); // MSB first, regardless of host byte order
    REQUIRE(e_buffer[1] == 0x02);
    REQUIRE(e_buffer[2] == 0x03);
    REQUIRE(e_buffer[3] == 0x04);
}

TEST_CASE("little-endian (the default) writes least-significant byte first") {
    e_clear();
    std::uint32_t v = 0x01020304u;
    serialize(v).to(e_buffer); // default endianness::little
    REQUIRE(e_buffer[0] == 0x04);
    REQUIRE(e_buffer[1] == 0x03);
    REQUIRE(e_buffer[2] == 0x02);
    REQUIRE(e_buffer[3] == 0x01);
}

TEST_CASE("big-endian round-trips through serialize/deserialize") {
    e_clear();
    std::uint32_t a = 0xDEADBEEFu;
    std::int16_t b = -12345;
    float c = 3.14159f;

    serialize<endianness::big>(a, b, c).to(e_buffer);

    auto fields = deserialize<endianness::big>(e_buffer).to<std::tuple<std::uint32_t, std::int16_t, float>>();
    REQUIRE(fields);
    auto& [ra, rb, rc] = *fields;
    REQUIRE(ra == a);
    REQUIRE(rb == b);
    REQUIRE(rc == Catch::Approx(c));
}

TEST_CASE("endianness mismatch yields byte-swapped data") {
    e_clear();
    std::uint32_t v = 0x01020304u;
    serialize<endianness::big>(v).to(e_buffer);              // wrote big-endian
    auto wrong = deserialize(e_buffer).to<std::uint32_t>();  // read as little (default)
    REQUIRE(wrong);
    REQUIRE(*wrong == 0x04030201u);                          // observably swapped
}

TEST_CASE("big-endian array converts each element independently") {
    e_clear();
    std::uint16_t arr[3] = {0x0102, 0x0304, 0x0506};
    serialize<endianness::big>(arr).to(e_buffer);

    // Each 2-byte element is MSB-first; elements stay in order (not reversed across the array).
    REQUIRE(e_buffer[0] == 0x01); REQUIRE(e_buffer[1] == 0x02);
    REQUIRE(e_buffer[2] == 0x03); REQUIRE(e_buffer[3] == 0x04);
    REQUIRE(e_buffer[4] == 0x05); REQUIRE(e_buffer[5] == 0x06);

    auto out = deserialize<endianness::big>(e_buffer).to<std::array<std::uint16_t, 3>>();
    REQUIRE(out);
    REQUIRE((*out)[0] == 0x0102);
    REQUIRE((*out)[1] == 0x0304);
    REQUIRE((*out)[2] == 0x0506);
}

TEST_CASE("big-endian fixed_string round-trips (endianness-neutral)") {
    e_clear();
    fixed_string<16> name{"node-7"};

    serialize<endianness::big>(name).to(e_buffer);

    // A fixed_string is byte-only, so the wire bytes are the characters verbatim under any order.
    REQUIRE(e_buffer[0] == static_cast<std::uint8_t>('n'));
    REQUIRE(e_buffer[1] == static_cast<std::uint8_t>('o'));
    REQUIRE(e_buffer[5] == static_cast<std::uint8_t>('7'));

    auto r = deserialize<endianness::big>(e_buffer).to<fixed_string<16>>();
    REQUIRE(r);
    REQUIRE(r->view() == "node-7");
}

TEST_CASE("big-endian string field travels alongside swapped scalars") {
    e_clear();
    std::uint32_t id = 0x01020304u;
    fixed_string<8> tag{"hi"};

    serialize<endianness::big>(id, tag).to(e_buffer);
    REQUIRE(e_buffer[0] == 0x01);            // scalar is byte-swapped
    REQUIRE(e_buffer[4] == static_cast<std::uint8_t>('h')); // string is not

    auto fields = deserialize<endianness::big>(e_buffer).to<std::tuple<std::uint32_t, fixed_string<8>>>();
    REQUIRE(fields);
    auto& [rid, rtag] = *fields;
    REQUIRE(rid == id);
    REQUIRE(rtag.view() == "hi");
}

TEST_CASE("Nested std::array round-trips big-endian (recursive swap)") {
    e_clear();
    std::array<std::array<std::uint16_t, 2>, 2> nested = {{ {{0x0102, 0x0304}}, {{0x0506, 0x0708}} }};

    serialize<endianness::big>(nested).to(e_buffer);
    // Each inner scalar is MSB-first; elements keep their order across both nesting levels.
    REQUIRE(e_buffer[0] == 0x01); REQUIRE(e_buffer[1] == 0x02);
    REQUIRE(e_buffer[2] == 0x03); REQUIRE(e_buffer[3] == 0x04);
    REQUIRE(e_buffer[6] == 0x07); REQUIRE(e_buffer[7] == 0x08);

    auto out = deserialize<endianness::big>(e_buffer).to<std::array<std::array<std::uint16_t, 2>, 2>>();
    REQUIRE(out);
    REQUIRE(*out == nested);
}

TEST_CASE("explicit little-endian equals the default") {
    e_clear();
    std::uint8_t a[EBUF] = {0};
    std::uint8_t b[EBUF] = {0};
    std::uint32_t v = 0xCAFEBABEu;

    serialize(v).to(a);                     // default
    serialize<endianness::little>(v).to(b); // explicit little
    REQUIRE(std::equal(a, a + 4, b));
}
