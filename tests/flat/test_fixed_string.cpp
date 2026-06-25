#include <catch2/catch_all.hpp>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include "eser/flat/serializer.hpp"
#include "eser/flat/deserializer.hpp"
#include "eser/tools/fixed_string.hpp"

using namespace eser::flat;
using namespace eser::tools;

constexpr std::size_t BUF = 64;
static std::uint8_t fs_buffer[BUF];

void fs_clear() {
    std::fill(fs_buffer, fs_buffer + BUF, 0xAB);
}

TEST_CASE("fixed_string is a trivially-copyable N-byte value") {
    static_assert(std::is_trivially_copyable_v<fixed_string<16>>,
                  "fixed_string must ride the trivially-copyable codec paths");
    static_assert(sizeof(fixed_string<16>) == 16, "fixed_string occupies exactly N bytes");
    static_assert(fixed_string<16>::capacity() == 16, "capacity reports N");
}

TEST_CASE("fixed_string view trims at the first null") {
    fixed_string<8> s{"hi"};
    REQUIRE(s.view() == "hi");
    REQUIRE(s.size() == 2);
}

TEST_CASE("fixed_string data() exposes the raw, padded buffer") {
    fixed_string<8> s{"hi"};
    const char* p = s.data();
    REQUIRE(p[0] == 'h');
    REQUIRE(p[1] == 'i');
    REQUIRE(p[2] == '\0');           // null-padded since "hi" is shorter than N
    REQUIRE(p == s.data());          // O(1), stable pointer into the object
    REQUIRE(std::string_view(p, s.size()) == "hi");
}

TEST_CASE("fixed_string filling all N bytes has no terminator but still views fully") {
    fixed_string<5> s{"hello"}; // exactly 5 chars, no room for a '\0'
    REQUIRE(s.view() == "hello");
    REQUIRE(s.size() == 5);
}

#ifdef NDEBUG
// A source longer than the capacity asserts in debug; in release it truncates to N.
TEST_CASE("fixed_string truncates a source longer than its capacity (release)") {
    fixed_string<3> s{"hello"};
    REQUIRE(s.view() == "hel");
    REQUIRE(s.size() == 3);
}
#endif

TEST_CASE("fixed_string does not require a null-terminated source") {
    // A string_view over a non-null-terminated range: raw[3] is 'd', not '\0'.
    const char raw[] = {'a', 'b', 'c', 'd'};
    std::string_view unterminated(raw, 3); // "abc", no terminator in the source

    fixed_string<8> s{unterminated};
    REQUIRE(s.view() == "abc");       // copied by length, not by scanning the source
    REQUIRE(s.size() == 3);
    REQUIRE(s.data()[3] == '\0');     // terminator comes from our zero-padding, not the source

    // Same, but the content fills the whole field: no terminator, still correct.
    fixed_string<5> full{std::string_view(raw, 4)}; // only 4 chars available; N=5 leaves 1 pad
    REQUIRE(full.view() == "abcd");
    REQUIRE(full.size() == 4);
}

TEST_CASE("fixed_string round-trips through the codec") {
    fs_clear();

    SECTION("short string is null-padded and recovered") {
        fixed_string<16> original{"sensor"};
        auto written = serialize(original).to(fs_buffer);
        REQUIRE(written == 16);

        auto result = deserialize(fs_buffer).to<fixed_string<16>>();
        REQUIRE(result);
        REQUIRE(result->view() == "sensor");
        REQUIRE(*result == original);
    }

    SECTION("capacity-filling string round-trips with no terminator") {
        fixed_string<5> original{"hello"};
        serialize(original).to(fs_buffer);

        auto result = deserialize(fs_buffer).to<fixed_string<5>>();
        REQUIRE(result);
        REQUIRE(result->view() == "hello");
    }
}

TEST_CASE("fixed_string composes inside a tuple with other fields") {
    fs_clear();
    std::uint32_t id = 1234;
    fixed_string<12> name{"node-a"};
    float value = 3.5f;

    serialize(id, name, value).to(fs_buffer);

    auto result = deserialize(fs_buffer).to<std::tuple<std::uint32_t, fixed_string<12>, float>>();
    REQUIRE(result);
    auto& [rid, rname, rvalue] = *result;
    REQUIRE(rid == id);
    REQUIRE(rname.view() == "node-a");
    REQUIRE(rvalue == Catch::Approx(value));
}

TEST_CASE("fixed_string deserialization respects the strict length contract") {
    fs_clear();
    // A 16-byte field cannot be read from a 15-byte buffer.
    auto result = deserialize(fs_buffer, 15).to<fixed_string<16>>();
    REQUIRE_FALSE(result.has_value());
}
