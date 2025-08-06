cmake_minimum_required(VERSION 3.20)
project(eser LANGUAGES CXX)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds are not allowed. Use: cmake -S . -B build")
endif()


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Interface library for header-only code
add_library(eser INTERFACE)

# Public include directory
target_include_directories(eser INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# Compiler warnings for consumer projects
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(eser INTERFACE -Wall -Wextra -pedantic)
endif()


# ---------------------------------
# Add testing toggle
# ---------------------------------
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)  # adds BUILD_TESTING option automatically
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()