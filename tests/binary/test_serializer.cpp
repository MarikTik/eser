#include <catch2/catch_test_macros.hpp>
#include "ser/binary/serializer.hpp"

using namespace ser::binary;

constexpr int BUFFER_SIZE = 200;
std::uint8_t buffer[BUFFER_SIZE];
constexpr std::uint8_t mask = 0xFF;

void zero_buffer() {
    for (int i = 0; i < BUFFER_SIZE; ++i) buffer[i] = 0;
}

void fill_buffer() {
    for (int i = 0; i < BUFFER_SIZE; ++i) buffer[i] = 0xAB;
}

TEST_CASE("Serialize uint8_t") {
    zero_buffer();
    std::uint8_t a = 12, b = 255, c = 34, d = 78;
    auto written = serialize(a, b, c, d).to(buffer);
    REQUIRE(written == 4);
    REQUIRE(buffer[0] == a);
    REQUIRE(buffer[1] == b);
    REQUIRE(buffer[2] == c);
    REQUIRE(buffer[3] == d);
}

TEST_CASE("Serialize uint16_t") {
    zero_buffer();
    std::uint16_t a = 1234, b = 65535, c = 400;
    auto written = serialize(a, b, c).to(buffer);
    REQUIRE(written == 6);
    REQUIRE(buffer[0] == (a & mask));
    REQUIRE(buffer[1] == (a >> 8));
    REQUIRE(buffer[2] == (b & mask));
    REQUIRE(buffer[3] == (b >> 8));
    REQUIRE(buffer[4] == (c & mask));
    REQUIRE(buffer[5] == (c >> 8));
}

TEST_CASE("Serialize uint32_t") {
    zero_buffer();
    std::uint32_t a = 0xFFFFFFFF, b = 123456789;
    auto written = serialize(a, b).to(buffer);
    REQUIRE(written == 8);
    for (int i = 0; i < 4; ++i) REQUIRE(buffer[i] == ((a >> (8 * i)) & mask));
    for (int i = 0; i < 4; ++i) REQUIRE(buffer[4 + i] == ((b >> (8 * i)) & mask));
}

TEST_CASE("Serialize int8_t") {
    zero_buffer();
    std::int8_t a = -12, b = 127;
    auto written = serialize(a, b).to(buffer);
    REQUIRE(written == 2);
    REQUIRE(buffer[0] == (a & mask));
    REQUIRE(buffer[1] == (b & mask));
}

TEST_CASE("Serialize int16_t") {
    zero_buffer();
    std::int16_t a = -1234, b = 32767;
    auto written = serialize(a, b).to(buffer);
    REQUIRE(written == 4);
    REQUIRE(buffer[0] == (a & mask));
    REQUIRE(buffer[1] == ((a >> 8) & mask));
    REQUIRE(buffer[2] == (b & mask));
    REQUIRE(buffer[3] == ((b >> 8) & mask));
}

TEST_CASE("Serialize int32_t") {
    zero_buffer();
    std::int32_t a = -123456, b = 2147483647, c = -98765;
    auto written = serialize(a, b, c).to(buffer);
    REQUIRE(written == 12);
    for (int i = 0; i < 4; ++i) REQUIRE(buffer[i] == ((a >> (8 * i)) & mask));
    for (int i = 0; i < 4; ++i) REQUIRE(buffer[4 + i] == ((b >> (8 * i)) & mask));
    for (int i = 0; i < 4; ++i) REQUIRE(buffer[8 + i] == ((c >> (8 * i)) & mask));
}

TEST_CASE("Serialize int64_t") {
    zero_buffer();
    std::int64_t a = -1234567890123456789LL;
    std::int64_t b = 9223372036854775807LL;
    std::int64_t c = -112233445566778899LL;
    auto written = serialize(a, b, c).to(buffer);
    REQUIRE(written == 24);
    for (int i = 0; i < 8; ++i) REQUIRE(buffer[i] == ((a >> (8 * i)) & mask));
    for (int i = 0; i < 8; ++i) REQUIRE(buffer[8 + i] == ((b >> (8 * i)) & mask));
    for (int i = 0; i < 8; ++i) REQUIRE(buffer[16 + i] == ((c >> (8 * i)) & mask));
}

TEST_CASE("Serialize float") {
    zero_buffer();
    float a = 3.14f, b = -1.23f;
    auto written = serialize(a, b).to(buffer);
    REQUIRE(written == sizeof(float) * 2);
    auto fa = *reinterpret_cast<std::uint32_t*>(&a);
    auto fb = *reinterpret_cast<std::uint32_t*>(&b);
    for (int i = 0; i < 4; ++i) REQUIRE(buffer[i] == ((fa >> (8 * i)) & mask));
    for (int i = 0; i < 4; ++i) REQUIRE(buffer[4 + i] == ((fb >> (8 * i)) & mask));
}

TEST_CASE("Serialize double") {
    zero_buffer();
    double a = 3.14159, b = -2.71828;
    auto written = serialize(a, b).to(buffer);
    REQUIRE(written == sizeof(double) * 2);
    auto da = *reinterpret_cast<std::uint64_t*>(&a);
    auto db = *reinterpret_cast<std::uint64_t*>(&b);
    for (int i = 0; i < 8; ++i) REQUIRE(buffer[i] == ((da >> (8 * i)) & mask));
    for (int i = 0; i < 8; ++i) REQUIRE(buffer[8 + i] == ((db >> (8 * i)) & mask));
}

TEST_CASE("Serialize bool") {
    zero_buffer();
    bool a = true, b = false, c = true;
    auto written = serialize(a, b, c).to(buffer);
    REQUIRE(written == 3);
    REQUIRE(buffer[0] == static_cast<std::uint8_t>(a));
    REQUIRE(buffer[1] == static_cast<std::uint8_t>(b));
    REQUIRE(buffer[2] == static_cast<std::uint8_t>(c));
}

TEST_CASE("Serialize array of int32_t") {
    zero_buffer();
    std::int32_t arr[3] = {1, -1, 42};
    auto written = serialize(arr).to(buffer);
    REQUIRE(written == sizeof(arr));
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j)
            REQUIRE(buffer[i * 4 + j] == ((arr[i] >> (8 * j)) & mask));
}

TEST_CASE("Serialize enum values") {
    zero_buffer();
    enum class E1 : std::uint32_t { A = 1111, B, C };
    enum class E2 : std::int16_t { A = -10, B, C };
    enum E3 : std::int8_t { X = 120, Y };

    auto e1 = E1::A;
    auto e2 = E2::C;
    auto e3 = E3::Y;
    auto written = serialize(e1, e2, e3).to(buffer);
    REQUIRE(written == 7);
    auto v1 = static_cast<std::uint32_t>(e1);
    auto v2 = static_cast<std::int16_t>(e2);
    auto v3 = static_cast<std::int8_t>(e3);
    for (int i = 0; i < 4; ++i) REQUIRE(buffer[i] == ((v1 >> (8 * i)) & mask));
    for (int i = 0; i < 2; ++i) REQUIRE(buffer[4 + i] == ((v2 >> (8 * i)) & mask));
    REQUIRE(buffer[6] == (v3 & mask));
}

TEST_CASE("Serialize C-string") {
    zero_buffer();
    const char str[] = "TestingString";
    serialize(str).to(buffer, BUFFER_SIZE);
    for (size_t i = 0; i <= strlen(str); ++i)
        REQUIRE(buffer[i] == static_cast<std::uint8_t>(str[i]));
}

TEST_CASE("Serialize mixed types") {
    zero_buffer();
    char c = 'X'; bool b = true; std::int32_t i = 42; float f = 3.14f; double d = 2.71828;
    std::int16_t arr[3] = {1, -1, 42};
    auto written = serialize(c, b, i, f, d, arr).to(buffer);
    REQUIRE(written == sizeof(c) + sizeof(b) + sizeof(i) + sizeof(f) + sizeof(d) + sizeof(arr));
    std::uint32_t fbits = *reinterpret_cast<std::uint32_t*>(&f);
    std::uint64_t dbits = *reinterpret_cast<std::uint64_t*>(&d);
    REQUIRE(buffer[0] == (c & mask));
    REQUIRE(buffer[1] == (b & mask));
    for (int k = 0; k < 4; ++k) REQUIRE(buffer[2 + k] == ((i >> (8 * k)) & mask));
    for (int k = 0; k < 4; ++k) REQUIRE(buffer[6 + k] == ((fbits >> (8 * k)) & mask));
    for (int k = 0; k < 8; ++k) REQUIRE(buffer[10 + k] == ((dbits >> (8 * k)) & mask));
    for (int j = 0; j < 3; ++j)
        for (int k = 0; k < 2; ++k)
            REQUIRE(buffer[18 + j * 2 + k] == ((arr[j] >> (8 * k)) & mask));
}

// TEST_CASE("Insufficient buffer causes assertion or failure") { // asserts false as intended
//     std::uint8_t small_buffer[2];
//     std::uint32_t data = 123456;
//     // Depending on your error handling, this may throw, assert, or silently fail
//     // Wrap with try-catch if your implementation uses exceptions.
//     REQUIRE_THROWS(serialize(data).to(small_buffer, sizeof(small_buffer)));
// }

TEST_CASE("Non-zero-initialized buffer remains intact outside serialized range") {
    fill_buffer(); // Should set whole buffer to 0xAB
    std::uint16_t val = 0xBEEF;
    serialize(val).to(buffer, 2); // Restrict write to 2 bytes
    REQUIRE(buffer[2] == 0xAB);
    REQUIRE(buffer[3] == 0xAB);
}

struct AlignedStruct {
    char a;
    int b;
};

TEST_CASE("Trivially copyable struct with padding") {
    AlignedStruct s = {'X', 42};
    serialize(s).to(buffer);
    auto* deserialized = reinterpret_cast<const AlignedStruct*>(buffer);
    REQUIRE(deserialized->a == 'X');
    REQUIRE(deserialized->b == 42);
}



TEST_CASE("Special float values: NaN and Infinity") {
    float nan = std::numeric_limits<float>::quiet_NaN();
    float inf = std::numeric_limits<float>::infinity();
    serialize(nan, inf).to(buffer);
    // Serialization completes successfully without crash
    REQUIRE(true);
}

TEST_CASE("Mixed-type array serialization") {
    std::uint8_t arr1[3] = {1, 2, 3};
    std::uint32_t arr2[2] = {12345, 67890};
    std::size_t size = serialize(arr1, arr2).to(buffer);
    REQUIRE(size == sizeof(arr1) + sizeof(arr2));
}

TEST_CASE("Large struct serialization near buffer limit") {
    struct LargeStruct {
        std::uint8_t data[200] = {0};
    } s;
    std::size_t size = serialize(s).to(buffer);
    REQUIRE(size == sizeof(s));
}
