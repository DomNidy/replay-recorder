# Find Google Test using vcpkg (CONFIG mode)
find_package(GTest CONFIG REQUIRED)

# Add new executable target(s) (for tests)
# TODO: Currently we're just building different binaries for each test, this is 
# TODO: prob unnecessary and we could just link them all into a single binary.
# TODO: Need to figure out how to use gtest!
# TODO: Also, we should probably come up with a more maintainable way of specifying
# TODO: The paths to the .cpp code we want to test, maybe create libs of them and
# TODO: Link those?

# Tests for event sink
add_executable(event_sink_tests event_sink_tests.cpp)

# Tests for RLE
add_executable(rle_tests rle_tests.cpp ../encoder/rle.cpp)

# Link test executable to the test code libraries and GTest 
target_link_libraries(event_sink_tests
    PRIVATE project_options GTest::gtest_main GTest::gmock_main
)
target_link_libraries(rle_tests
    PRIVATE project_options GTest::gtest_main GTest::gmock_main
)

# Add tests to CTest
add_test(NAME EventSinkTests COMMAND event_sink_tests)
add_test(NAME RLETests COMMAND rle_tests)

