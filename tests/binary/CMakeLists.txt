add_library(binary_tests OBJECT
    test_serializer.cpp
    test_deserializer.cpp
    test_integration.cpp
)

target_link_libraries(binary_tests PRIVATE ser_binary Catch2::Catch2WithMain)
target_include_directories(binary_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
