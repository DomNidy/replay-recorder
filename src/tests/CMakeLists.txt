# Find Google Test using vcpkg (CONFIG mode)
find_package(GTest CONFIG REQUIRED)

add_executable(event_sink_tests event_sink_tests.cpp)
add_executable(rle_tests rle_tests.cpp "${PROJECT_SOURCE_DIR}/src/encoder/rle.cpp")

# Link test executable to the test code libraries and GTest 
target_link_libraries(event_sink_tests
    PRIVATE project_options GTest::gtest_main GTest::gmock_main
)
target_link_libraries(rle_tests
    PRIVATE project_options replay_encoder_options GTest::gtest_main GTest::gmock_main
)

# Add tests to CTest
add_test(NAME EventSinkTests COMMAND event_sink_tests)
add_test(NAME RLETests COMMAND rle_tests)

