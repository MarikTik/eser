/**
* @file byte.hpp
* @ingroup ser_tools
* @brief Fallback mechanism for `std::byte` in embedded or incomplete toolchains.
*
* This header ensures the availability of `std::byte` for all platforms targeted by the
* `ser` serialization library and the broader `etask` framework.
*
* C++17 introduced `std::byte` as a strongly-typed, byte-sized type for representing raw data.
* However, many embedded or specialized toolchains may:
*   - Lack `<cstddef>` entirely
*   - Provide a partial `<cstddef>` missing `std::byte`
*
* This header performs the following:
*
*   - Verifies that the compiler supports C++17 or newer.
*   - Attempts to include `<cstddef>`.
*   - Detects whether `std::byte` is defined.
*   - If missing, defines `std::byte` as:
*     @code
*     namespace std {
*         enum class byte : unsigned char {};
*     }
*     @endcode
*
* @note
* This header modifies the `std` namespace only if `std::byte` is absent.
* This is generally discouraged but accepted here for compatibility in environments
* where upgrading the toolchain may not be feasible.
*
* Users are strongly encouraged to ensure proper C++17 toolchain support
* instead of relying on this fallback.
*/

#ifndef SER_TOOLS_BYTE_HPP_
#define SER_TOOLS_BYTE_HPP_

#if defined(__has_include)

    /**
    * @brief Verify compiler support for C++17.
    *
    * This library explicitly requires C++17 or newer.
    * Compilation will fail if an older standard is used.
    */
    #if __cplusplus < 201703L
        #error "[ser] C++17 is required for this library."
    #endif

    /**
    * @brief Attempt to include the standard `<cstddef>` header.
    *
    * If available, it may provide `std::byte`. If not,
    * the header defines a fallback.
    */
    #if __has_include(<cstddef>)
        #include <cstddef>

        /**
        * @brief Check for the presence of `std::byte`.
        *
        * If `__cpp_lib_byte` is undefined, assume `std::byte` is absent
        * and provide a fallback definition.
        */
        #ifndef __cpp_lib_byte
            /**
            * @namespace std
            * @brief Fallback definition of `std::byte` as a scoped enumeration.
            *
            * Some toolchains may omit `std::byte` despite claiming C++17 support.
            * This fallback ensures compatibility.
            */
            namespace std {
                enum class byte : unsigned char {};
            }
            #pragma message("[ser] <cstddef> found but std::byte missing. Defined fallback std::byte.")
        #endif // __cpp_lib_byte

    #else // __has_include(<cstddef>)

    /**
    * @brief Provide a complete fallback if `<cstddef>` is unavailable.
    *
    * This is rare but can occur on deeply embedded toolchains without
    * a full standard library implementation.
    */
    namespace std {
        enum class byte : unsigned char {};
    }

    #pragma message("[ser] <cstddef> not found. Defined fallback std::byte.")

    #endif // __has_include(<cstddef>)

#else 

    /**
    * @brief Handle compilers lacking `__has_include`.
    *
    * This block assumes `<cstddef>` is present, as there’s no portable
    * way to check header existence without `__has_include`.
    */
    #pragma message("[ser] Compiler does not support __has_include. Assuming <cstddef> is available, otherwise compilation will fail.")

    /**
    * If you don't have `<cstddef>` in general, you should not use this library, as many of its components rely
    * on cstddef components (std::byte, std::size_t, etc.).
    */
    #include <cstddef> // this will cause an error if <cstddef> is not available

    #ifndef __cpp_lib_byte
        namespace std {
            enum class byte : unsigned char {};
        }
        #pragma message("[ser] <cstddef> found but std::byte missing. Defined fallback std::byte.")
    #endif // __cpp_lib_byte

#endif // __has_include

#endif // SER_TOOLS_BYTE_HPP_
