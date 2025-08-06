add_executable(eser_tests

    test_serializer.cpp
    test_deserializer.cpp
    test_integration.cpp
)

target_link_libraries(eser_tests PRIVATE Catch2::Catch2WithMain eser)

add_test(NAME binary_tests COMMAND eser_tests)

#add_test(NAME all_tests COMMAND ser_tests)