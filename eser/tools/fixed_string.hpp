/**
* @file fixed_string.hpp
*
* @brief Fixed-capacity, trivially-copyable string field.
*
* @ingroup eser_tools
*
* This header defines `eser::tools::fixed_string`, an `N`-byte inline character buffer that
* represents a string as a fixed-size value. Because it is trivially copyable, it travels
* through the serializer/deserializer paths unchanged: serializing one writes exactly `N` bytes,
* and `deserialize(buffer).to<fixed_string<N>>()` reads them back.
*
* A `fixed_string<N>` always occupies `N` bytes — the capacity is part of the type, so the
* codec's compile-time size accounting keeps working. Shorter strings are null-padded; a string
* that fills all `N` bytes carries no terminator, which is why @ref fixed_string::view bounds its
* scan by `N`.
*
* @note There is deliberately no `c_str()`: a field filled to capacity has no null terminator.
*       Use @ref fixed_string::view for a bounded `std::string_view`, or @ref fixed_string::data
*       for a raw (not necessarily null-terminated) `const char*`.
*
* @author Mark Tikhonov <mtik.philosopher@gmail.com>
*
* @date 2026-06-24
*
* @copyright
* MIT License
* SPDX-License-Identifier: MIT
*
* @par Changelog
* - 2026-06-24
* -     Initial creation.
*/
#ifndef ESER_TOOLS_FIXED_STRING_HPP_
#define ESER_TOOLS_FIXED_STRING_HPP_
#include <cstddef>
#include <string_view>

namespace eser::tools{
    /**
    * @class fixed_string
    * @brief A fixed-capacity, trivially-copyable string occupying exactly `N` bytes.
    *
    * @tparam N The capacity in bytes (the on-the-wire size). Must be strictly positive.
    *
    * The characters are stored inline in a private `char[N]`. The type is trivially copyable and
    * `sizeof(fixed_string<N>) == N`, so it is serialized/deserialized by the same `memcpy` paths
    * as any other trivially-copyable value and the codec's compile-time size accounting still
    * holds. Read the contents either as a bounded `std::string_view` (@ref view, O(N)) or as a
    * raw `const char*` (@ref data, O(1)); the buffer is not necessarily null-terminated.
    *
    * The field carries no cached length: storing one would push `sizeof` past `N` and break the
    * fixed wire size, so the logical length is recomputed on each @ref view / @ref size call.
    *
    * ## Null-termination contract
    *
    * The source passed to the constructor need **not** be null-terminated — `std::string_view` is
    * a `(pointer, length)` pair with no terminator guarantee, and characters are copied by length,
    * never by scanning the source. The terminator that @ref view and @ref size rely on instead
    * comes from this type's own zero-padding: the buffer is zero-initialized before the copy, so
    * any bytes past the content are `\0`. Consequently a field whose content is shorter than `N`
    * is self-terminating, while a field whose content fills all `N` bytes has no terminator — which
    * is why @ref view bounds its scan by `N` and @ref data is documented as possibly unterminated.
    */
    template<std::size_t N>
    class fixed_string{
        static_assert(N > 0, "fixed_string capacity N must be strictly positive");

    public:
        /**
        * @brief Construct an empty field with all `N` bytes set to zero.
        *
        * @post `size() == 0` and `view().empty()`.
        */
        constexpr fixed_string() noexcept;

        /**
        * @brief Construct from a string view, copying up to `N` characters.
        *
        * Copies `min(source.size(), N)` characters into the field; any remaining bytes are left
        * zero. A source longer than `N` is truncated to the capacity.
        *
        * @param source The characters to store. A `const char*` or string literal converts
        *               implicitly (through `std::string_view`).
        *
        * @pre `source.size() <= N`. A longer source is truncated: the overflow is flagged by
        *      `assert` in debug builds and silently truncated under `NDEBUG`.
        * @note The field is null-padded only when `source.size() < N`; a source of exactly `N`
        *       characters leaves no terminator (see @ref data).
        */
        constexpr fixed_string(std::string_view source) noexcept;

        /**
        * @brief A bounded view over the logical string contents.
        *
        * The view spans up to the first null byte, or the whole `N`-byte field when the string
        * fills it with no terminator.
        *
        * @return A `std::string_view` covering the characters up to the first null (length `<= N`).
        *
        * @note O(N): the logical length is recomputed by scanning on every call — the type stores
        *       no cached length. For O(1) raw access use @ref data.
        * @warning The returned view points into this object and must not outlive it.
        */
        [[nodiscard]] constexpr std::string_view view() const noexcept;

        /**
        * @brief Pointer to the underlying `N`-byte character storage.
        *
        * An O(1) accessor that performs no scan — useful when the length is tracked separately or
        * when handing the buffer to a C API that consumes `const char*`. This is the same contract
        * as `std::string_view::data()`.
        *
        * @return A read-only pointer to the first of the `N` stored bytes.
        *
        * @warning The buffer is **not guaranteed to be null-terminated**: a string that fills all
        *          `N` bytes carries no terminator. Bound any read by @ref size or @ref capacity,
        *          or use @ref view, rather than treating the result as a C string.
        * @note The pointer refers into this object and is invalidated when it is destroyed or
        *       reassigned.
        */
        [[nodiscard]] constexpr const char* data() const noexcept;

        /**
        * @brief The logical length of the string: characters up to the first null, at most `N`.
        *
        * @return The logical length in bytes.
        *
        * @note O(N): equivalent to `view().size()`; the length is recomputed on each call.
        */
        [[nodiscard]] constexpr std::size_t size() const noexcept;

        /**
        * @brief The fixed capacity `N` — the number of bytes the field always occupies on the wire.
        *
        * @return The compile-time capacity `N`.
        */
        [[nodiscard]] static constexpr std::size_t capacity() noexcept;

        /**
        * @brief Equality by logical string contents.
        *
        * Two fields compare equal when their @ref view "views" are equal — the same characters up
        * to the first null. Trailing bytes past the terminator are ignored.
        *
        * @param other The field to compare against.
        * @return `true` if both fields hold the same logical string, `false` otherwise.
        *
        * @note O(N): both operands are scanned to their logical length.
        */
        [[nodiscard]] constexpr bool operator==(const fixed_string& other) const noexcept;

        /**
        * @brief Inequality by logical string contents; the negation of `operator==`.
        *
        * @param other The field to compare against.
        * @return `true` if the fields hold different logical strings, `false` otherwise.
        */
        [[nodiscard]] constexpr bool operator!=(const fixed_string& other) const noexcept;

    private:
        char _data[N]; ///< Inline character storage; bytes past the logical length are zero unless the string fills the field.
    };
} // namespace eser::tools

#include "fixed_string.tpp"
#endif // ESER_TOOLS_FIXED_STRING_HPP_
