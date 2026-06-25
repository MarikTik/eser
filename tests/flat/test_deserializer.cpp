#include <catch2/catch_all.hpp>
#include <limits>
#include <cstdint>
#include <tuple>
#include <array>
#include "eser/flat/deserializer.hpp"

using namespace eser::flat;

constexpr std::size_t BUFFER_SIZE = 200;
static std::uint8_t buffer[BUFFER_SIZE];
constexpr std::uint8_t mask = 0xFF;

void fill() {
    std::fill(buffer, buffer + BUFFER_SIZE, 0);
}

TEST_CASE("Deserialize uint8_t values") {
    fill();
    std::uint8_t v1 = 12, v2 = 255, v3 = 34, v4 = 78;
    buffer[0] = v1; buffer[1] = v2; buffer[2] = v3; buffer[3] = v4;

    auto result = deserialize(buffer).to<std::tuple<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t>>();

    REQUIRE(result);
    auto& [a, b, c, d] = *result;
    REQUIRE(a == v1);
    REQUIRE(b == v2);
    REQUIRE(c == v3);
    REQUIRE(d == v4);
}

TEST_CASE("Deserialize uint16_t values") {
    fill();
    std::uint16_t v1 = 1234, v2 = 65535, v3 = 400;
    buffer[0] = v1 & mask; buffer[1] = v1 >> 8;
    buffer[2] = v2 & mask; buffer[3] = v2 >> 8;
    buffer[4] = v3 & mask; buffer[5] = v3 >> 8;

    auto result = deserialize(buffer).to<std::tuple<std::uint16_t, std::uint16_t, std::uint16_t>>();

    REQUIRE(result);
    auto& [a, b, c] = *result;
    REQUIRE(a == v1);
    REQUIRE(b == v2);
    REQUIRE(c == v3);
}

TEST_CASE("Deserialize uint32_t values") {
    fill();
    std::uint32_t v1 = 4004967295U, v2 = 123456789;
    for (int i = 0; i < 4; ++i) buffer[i] = v1 >> (8 * i);
    for (int i = 0; i < 4; ++i) buffer[4 + i] = v2 >> (8 * i);

    auto result = deserialize(buffer).to<std::tuple<std::uint32_t, std::uint32_t>>();

    REQUIRE(result);
    auto& [a, b] = *result;
    REQUIRE(a == v1);
    REQUIRE(b == v2);
}

TEST_CASE("Deserialize uint64_t value") {
    fill();
    std::uint64_t v = 1234567890123456789ULL;
    for (int i = 0; i < 8; ++i) buffer[i] = v >> (8 * i);

    auto a = deserialize(buffer).to<std::uint64_t>();

    REQUIRE(a);
    REQUIRE(*a == v);
}

TEST_CASE("Deserialize int8_t values") {
    fill();
    std::int8_t v1 = -12, v2 = 127;
    buffer[0] = v1; buffer[1] = v2;

    auto result = deserialize(buffer).to<std::tuple<std::int8_t, std::int8_t>>();

    REQUIRE(result);
    auto& [a, b] = *result;
    REQUIRE(a == v1);
    REQUIRE(b == v2);
}

TEST_CASE("Deserialize int16_t values") {
    fill();
    std::int16_t v1 = -1234, v2 = 32767, v3 = -32111;
    buffer[0] = v1 & mask; buffer[1] = v1 >> 8;
    buffer[2] = v2 & mask; buffer[3] = v2 >> 8;
    buffer[4] = v3 & mask; buffer[5] = v3 >> 8;

    auto result = deserialize(buffer).to<std::tuple<std::int16_t, std::int16_t, std::int16_t>>();

    REQUIRE(result);
    auto& [a, b, c] = *result;
    REQUIRE(a == v1);
    REQUIRE(b == v2);
    REQUIRE(c == v3);
}

TEST_CASE("Deserialize int32_t values") {
    fill();
    std::int32_t v1 = -123456, v2 = 2147483647, v3 = -987654, v4 = 10000000;
    std::int32_t vals[] = {v1, v2, v3, v4};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            buffer[i * 4 + j] = vals[i] >> (8 * j);

    auto result = deserialize(buffer).to<std::tuple<std::int32_t, std::int32_t, std::int32_t, std::int32_t>>();

    REQUIRE(result);
    auto& [a, b, c, d] = *result;
    REQUIRE(a == v1);
    REQUIRE(b == v2);
    REQUIRE(c == v3);
    REQUIRE(d == v4);
}

TEST_CASE("Deserialize int64_t values") {
    fill();
    std::int64_t v1 = -1234567890123456789LL;
    std::int64_t v2 = 9223372036854775807LL;
    std::int64_t v3 = -987654321098765432LL;
    std::int64_t v4 = 1234;
    std::int64_t vals[] = {v1, v2, v3, v4};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 8; ++j)
            buffer[i * 8 + j] = vals[i] >> (8 * j);

    auto result = deserialize(buffer).to<std::tuple<std::int64_t, std::int64_t, std::int64_t, std::int64_t>>();

    REQUIRE(result);
    auto& [a, b, c, d] = *result;
    REQUIRE(a == v1);
    REQUIRE(b == v2);
    REQUIRE(c == v3);
    REQUIRE(d == v4);
}

TEST_CASE("Deserialize float values") {
    fill();
    float v[] = {3.14159f, -2.71828f, 1.61803f, 14.333f, -13.9564f};
    for (int i = 0; i < 5; ++i) {
        auto bits = *reinterpret_cast<std::uint32_t*>(&v[i]);
        for (int j = 0; j < 4; ++j)
            buffer[i * 4 + j] = bits >> (8 * j);
    }

    auto result = deserialize(buffer).to<std::tuple<float, float, float, float, float>>();

    REQUIRE(result);
    auto& [a, b, c, d, e] = *result;
    REQUIRE(a == Catch::Approx(v[0]));
    REQUIRE(b == Catch::Approx(v[1]));
    REQUIRE(c == Catch::Approx(v[2]));
    REQUIRE(d == Catch::Approx(v[3]));
    REQUIRE(e == Catch::Approx(v[4]));
}

TEST_CASE("Deserialize double values") {
    fill();
    double d1 = 3.141592653589793;
    double d2 = -2.718281828459045;
    std::uint64_t bits1 = *reinterpret_cast<std::uint64_t*>(&d1);
    std::uint64_t bits2 = *reinterpret_cast<std::uint64_t*>(&d2);
    for (int i = 0; i < 8; ++i) {
        buffer[i] = bits1 >> (8 * i);
        buffer[8 + i] = bits2 >> (8 * i);
    }

    auto result = deserialize(buffer).to<std::tuple<double, double>>();

    REQUIRE(result);
    auto& [a, b] = *result;
    REQUIRE(a == Catch::Approx(d1));
    REQUIRE(b == Catch::Approx(d2));
}

TEST_CASE("Deserialize bool values") {
    fill();
    bool vals[] = {true, false, true, false};
    for (int i = 0; i < 4; ++i)
        buffer[i] = static_cast<std::uint8_t>(vals[i]);

    auto result = deserialize(buffer).to<std::tuple<bool, bool, bool, bool>>();

    REQUIRE(result);
    auto& [a, b, c, d] = *result;
    REQUIRE(a == vals[0]);
    REQUIRE(b == vals[1]);
    REQUIRE(c == vals[2]);
    REQUIRE(d == vals[3]);
}

TEST_CASE("Deserialize int32_t[3] and int16_t[1] arrays") {
    fill();
    std::int32_t arr1[3] = {42, -42, 1000};
    std::int16_t arr2[1] = {258};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j)
            buffer[i * 4 + j] = arr1[i] >> (8 * j);
    buffer[12] = arr2[0] & mask;
    buffer[13] = arr2[0] >> 8;

    auto result = deserialize(buffer).to<std::tuple<std::array<std::int32_t, 3>, std::array<std::int16_t, 1>>>();

    REQUIRE(result);
    auto& [a1, a2] = *result;
    REQUIRE(a1[0] == arr1[0]);
    REQUIRE(a1[1] == arr1[1]);
    REQUIRE(a1[2] == arr1[2]);
    REQUIRE(a2[0] == arr2[0]);
}

TEST_CASE("Deserialize mixed-type structure") {
    fill();
    char c = 'X';
    bool b = true;
    std::int32_t i = 42;
    float f = 3.14f;
    double d = 2.71828;
    std::int16_t arr[3] = {1, -1, 42};

    buffer[0] = static_cast<std::uint8_t>(c);
    buffer[1] = static_cast<std::uint8_t>(b);
    for (int j = 0; j < 4; ++j) buffer[2 + j] = i >> (8 * j);
    auto f_bits = *reinterpret_cast<std::uint32_t*>(&f);
    for (int j = 0; j < 4; ++j) buffer[6 + j] = f_bits >> (8 * j);
    auto d_bits = *reinterpret_cast<std::uint64_t*>(&d);
    for (int j = 0; j < 8; ++j) buffer[10 + j] = d_bits >> (8 * j);
    for (int j = 0; j < 3; ++j) {
        buffer[18 + j * 2] = arr[j] & mask;
        buffer[18 + j * 2 + 1] = arr[j] >> 8;
    }

    auto result = deserialize(buffer).to<std::tuple<char, bool, std::int32_t, float, double, std::array<std::int16_t, 3>>>();

    REQUIRE(result);
    auto& [rc, rb, ri, rf, rd, rarr] = *result;
    REQUIRE(rc == c);
    REQUIRE(rb == b);
    REQUIRE(ri == i);
    REQUIRE(rf == Catch::Approx(f));
    REQUIRE(rd == Catch::Approx(d));
    REQUIRE(rarr[0] == arr[0]);
    REQUIRE(rarr[1] == arr[1]);
    REQUIRE(rarr[2] == arr[2]);
}

TEST_CASE("Deserialize enum types") {
    fill();
    enum class int32_enum : std::int32_t { a = 2000111333, b, c };
    enum int16_enum : std::int16_t { A = 23000, B, C };

    int32_enum e1 = int32_enum::a;
    int16_enum e2 = int16_enum::B;
    auto e1_bits = static_cast<std::uint32_t>(e1);
    auto e2_bits = static_cast<std::uint16_t>(e2);
    for (int i = 0; i < 4; ++i) buffer[i] = e1_bits >> (8 * i);
    buffer[4] = e2_bits & mask;
    buffer[5] = e2_bits >> 8;

    auto result = deserialize(buffer).to<std::tuple<int32_enum, int16_enum>>();

    REQUIRE(result);
    auto& [r1, r2] = *result;
    REQUIRE(r1 == e1);
    REQUIRE(r2 == e2);
}

TEST_CASE("Strict length contract — nullopt when buffer is too short") {
    fill();

    SECTION("scalar one byte short") {
        auto r = deserialize(buffer, sizeof(std::uint32_t) - 1).to<std::uint32_t>();
        REQUIRE_FALSE(r.has_value());
    }

    SECTION("exact length succeeds") {
        auto r = deserialize(buffer, sizeof(std::uint32_t)).to<std::uint32_t>();
        REQUIRE(r.has_value());
    }

    SECTION("multi-type missing the last field") {
        constexpr std::size_t required = sizeof(std::uint32_t) + sizeof(std::uint16_t);
        auto r = deserialize(buffer, required - 1).to<std::tuple<std::uint32_t, std::uint16_t>>();
        REQUIRE_FALSE(r.has_value());
    }

    SECTION("array that only partially fits yields nullopt, not a zero-padded array") {
        // Only 2 of 3 int32 elements fit (12 bytes required, 11 available).
        auto r = deserialize(buffer, 3 * sizeof(std::int32_t) - 1).to<std::array<std::int32_t, 3>>();
        REQUIRE_FALSE(r.has_value());
    }
}

TEST_CASE("Named std::array is deserialized directly") {
    fill();
    std::int32_t vals[3] = {7, -8, 9};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j)
            buffer[i * 4 + j] = vals[i] >> (8 * j);

    auto arr = deserialize(buffer).to<std::array<std::int32_t, 3>>();

    REQUIRE(arr);
    REQUIRE((*arr)[0] == vals[0]);
    REQUIRE((*arr)[1] == vals[1]);
    REQUIRE((*arr)[2] == vals[2]);
}

TEST_CASE("Single-element tuple unpacks unambiguously") {
    fill();
    std::int32_t v = 1234567;
    for (int j = 0; j < 4; ++j) buffer[j] = v >> (8 * j);

    // The whole point of the serde-style API: a one-field read is spelled as a
    // one-element tuple, so `auto [x]` is well-defined and never confused with to<T>().
    auto result = deserialize(buffer).to<std::tuple<std::int32_t>>();

    REQUIRE(result);
    auto& [x] = *result;
    REQUIRE(x == v);
}
