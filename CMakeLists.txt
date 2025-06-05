cmake_minimum_required(VERSION 3.20)
project(ser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ser_binary INTERFACE)

target_include_directories(ser_binary INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include> # optional for installation later
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(ser_binary INTERFACE -Wall -Wextra -pedantic)
endif()

# -------------------------------
# TESTS
# -------------------------------
enable_testing()

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4 # stable release
)
FetchContent_MakeAvailable(Catch2)

add_subdirectory(tests)

add_executable(ser_tests
    tests/test_main.cpp
    $<TARGET_OBJECTS:binary_tests>
)

target_link_libraries(ser_tests PRIVATE Catch2::Catch2WithMain)
add_test(NAME all_tests COMMAND ser_tests)

