add_executable(ser_tests

    test_serializer.cpp
    test_deserializer.cpp
    test_integration.cpp
)

target_link_libraries(ser_tests PRIVATE Catch2::Catch2WithMain ser)

add_test(NAME binary_tests COMMAND ser_tests)

#add_test(NAME all_tests COMMAND ser_tests)