#include <catch2/catch_all.hpp>
#include "eser/flat/serializer.hpp"
#include "eser/flat/deserializer.hpp"
#include <limits>
#include <cstdint>
#include <tuple>
#include <array>
using namespace eser::flat;

constexpr std::size_t BUFFER_SIZE = 200;
static std::uint8_t buffer[BUFFER_SIZE];

void fill0() {
    std::fill(buffer, buffer + BUFFER_SIZE, 0);
}

TEST_CASE("Serialize and deserialize uint types") {
    fill0();
    SECTION("uint8_t") {
        std::uint8_t v = 10;
        serialize(v).to(buffer);
        auto r = deserialize(buffer).to<std::uint8_t>();
        REQUIRE(r);
        REQUIRE(*r == v);
    }

    SECTION("uint16_t") {
        std::uint16_t a = 1000, b = 2000;
        serialize(a, b).to(buffer);
        auto result = deserialize(buffer).to<std::tuple<std::uint16_t, std::uint16_t>>();
        REQUIRE(result);
        auto& [ra, rb] = *result;
        REQUIRE(ra == a);
        REQUIRE(rb == b);
    }

    SECTION("uint32_t") {
        std::uint32_t a = 100000, b = 200000, c = 300000;
        serialize(a, b, c).to(buffer);
        auto result = deserialize(buffer).to<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>>();
        REQUIRE(result);
        auto& [ra, rb, rc] = *result;
        REQUIRE(ra == a);
        REQUIRE(rb == b);
        REQUIRE(rc == c);
    }

    SECTION("uint64_t") {
        std::uint64_t a = 1e12, b = 2e12, c = 3e12, d = 4e12;
        serialize(a, b, c, d).to(buffer);
        auto result = deserialize(buffer).to<std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t>>();
        REQUIRE(result);
        auto& [ra, rb, rc, rd] = *result;
        REQUIRE(ra == a);
        REQUIRE(rb == b);
        REQUIRE(rc == c);
        REQUIRE(rd == d);
    }
}

TEST_CASE("Serialize and deserialize bool values") {
    fill0();
    bool a = true, b = false;
    serialize(a, b).to(buffer);
    auto result = deserialize(buffer).to<std::tuple<bool, bool>>();
    REQUIRE(result);
    auto& [ra, rb] = *result;
    REQUIRE(ra == a);
    REQUIRE(rb == b);
}

TEST_CASE("Serialize and deserialize signed ints") {
    fill0();
    SECTION("int8_t") {
        std::int8_t a = -10, b = 10;
        serialize(a, b).to(buffer);
        auto result = deserialize(buffer).to<std::tuple<std::int8_t, std::int8_t>>();
        REQUIRE(result);
        auto& [ra, rb] = *result;
        REQUIRE(ra == a);
        REQUIRE(rb == b);
    }

    SECTION("int16_t") {
        std::int16_t a = -1000;
        serialize(a).to(buffer);
        auto r = deserialize(buffer).to<std::int16_t>();
        REQUIRE(r);
        REQUIRE(*r == a);
    }

    SECTION("int32_t") {
        std::int32_t a = -100000, b = 100000, c = -200000;
        serialize(a, b, c).to(buffer);
        auto result = deserialize(buffer).to<std::tuple<std::int32_t, std::int32_t, std::int32_t>>();
        REQUIRE(result);
        auto& [ra, rb, rc] = *result;
        REQUIRE(ra == a);
        REQUIRE(rb == b);
        REQUIRE(rc == c);
    }

    SECTION("int64_t") {
        std::int64_t a = -1e12, b = 1e12;
        serialize(a, b).to(buffer);
        auto result = deserialize(buffer).to<std::tuple<std::int64_t, std::int64_t>>();
        REQUIRE(result);
        auto& [ra, rb] = *result;
        REQUIRE(ra == a);
        REQUIRE(rb == b);
    }
}

TEST_CASE("Serialize and deserialize float and double values") {
    fill0();
    SECTION("float") {
        float f = 3.14f;
        serialize(f).to(buffer);
        auto rf = deserialize(buffer).to<float>();
        REQUIRE(rf);
        REQUIRE(*rf == Catch::Approx(f));
    }

    SECTION("double") {
        double a = 3.14159, b = -2.71828, c = 1.61803;
        serialize(a, b, c).to(buffer);
        auto result = deserialize(buffer).to<std::tuple<double, double, double>>();
        REQUIRE(result);
        auto& [ra, rb, rc] = *result;
        REQUIRE(ra == Catch::Approx(a));
        REQUIRE(rb == Catch::Approx(b));
        REQUIRE(rc == Catch::Approx(c));
    }
}

TEST_CASE("serialize() captures arguments by value (no aliasing/dangling)") {
    fill0();
    int a = 42;
    auto s = serialize(a);   // owns a copy of a, independent of the source variable
    a = 99;                  // mutate the source after building the serializer
    s.to(buffer);
    auto r = deserialize(buffer).to<int>();
    REQUIRE(r);
    REQUIRE(*r == 42);       // the captured 42 is serialized, not the later 99
}

TEST_CASE("Serialize a std::array argument directly") {
    fill0();
    std::array<std::int32_t, 4> arr = {10, -20, 30, -40};
    serialize(arr).to(buffer);
    auto out = deserialize(buffer).to<std::array<std::int32_t, 4>>();
    REQUIRE(out);
    REQUIRE(*out == arr);
}

TEST_CASE("Nested std::array round-trips (recursive element handling)") {
    fill0();
    std::array<std::array<std::uint16_t, 2>, 3> nested = {{ {{1, 2}}, {{3, 4}}, {{5, 6}} }};
    serialize(nested).to(buffer);
    auto out = deserialize(buffer).to<std::array<std::array<std::uint16_t, 2>, 3>>();
    REQUIRE(out);
    REQUIRE(*out == nested);
}

TEST_CASE("Serialize and deserialize arrays") {
    fill0();
    const std::uint8_t a1[3] = {1, 2, 3};
    std::int32_t a2[4] = {-1444222555, 1444333222, -1, 2111999000};
    serialize(a1, a2).to(buffer);
    auto result = deserialize(buffer).to<std::tuple<std::array<std::uint8_t, 3>, std::array<std::int32_t, 4>>>();
    REQUIRE(result);
    auto& [ra1, ra2] = *result;
    for (int i = 0; i < 3; ++i) REQUIRE(ra1[i] == a1[i]);
    for (int i = 0; i < 4; ++i) REQUIRE(ra2[i] == a2[i]);
}

TEST_CASE("Serialize and deserialize mixed types") {
    fill0();
    std::uint32_t u = 123456789;
    bool b = true;
    std::int16_t i = -12345;
    float f = 3.14159f;
    std::uint8_t arr[3] = {1, 2, 3};
    serialize(u, b, i, f, arr).to(buffer);
    auto result = deserialize(buffer).to<std::tuple<std::uint32_t, bool, std::int16_t, float, std::array<std::uint8_t, 3>>>();
    REQUIRE(result);
    auto& [ru, rb, ri, rf, rarr] = *result;
    REQUIRE(ru == u);
    REQUIRE(rb == b);
    REQUIRE(ri == i);
    REQUIRE(rf == Catch::Approx(f));
    for (int i = 0; i < 3; ++i) REQUIRE(rarr[i] == arr[i]);
}

TEST_CASE("Serialize and deserialize enum class") {
    enum class test_enum : std::uint8_t {
        val1 = 1 | (1 << 3),
        val2 = (1 << 2) | (1 << 4),
        val3 = (1 << 5) | (1 << 6)
    };
    fill0();
    test_enum e = test_enum::val1;
    serialize(e).to(buffer);
    auto r = deserialize(buffer).to<test_enum>();
    REQUIRE(r);
    REQUIRE(*r == e);
}



TEST_CASE("Deserialize trivially copyable struct", "[deserializer][struct]") {
    struct point {
        int x;
        int y;

        bool operator==(const point& other) const {
            return x == other.x && y == other.y;
        }
    };
    std::byte buffer[sizeof(point)];
    static_assert(std::is_trivially_copyable_v<point>, "point must be trivially copyable");
    point original{42, -7};

    serialize(original).to(buffer);

    // Deserialize it back
    auto des = deserialize(buffer, sizeof(buffer));
    auto p = des.to<point>();

    REQUIRE(p);
    REQUIRE(original == *p);
    REQUIRE_FALSE(des.to<int>().has_value()); // Cursor advanced past the struct: no bytes left
}
