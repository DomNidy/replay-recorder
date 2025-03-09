# Find Google Test using vcpkg (CONFIG mode)
find_package(GTest CONFIG REQUIRED)

# Add new executable target(s) (for tests)
add_executable(event_sink_tests event_sink_tests.cpp)
target_include_directories(event_sink_tests PUBLIC "${PROJECT_SOURCE_DIR}/include")

# Link test executable to the test code libraries and GTest 
target_link_libraries(event_sink_tests PRIVATE GTest::gtest_main GTest::gmock_main)

# Add tests to CTest
add_test(NAME EventSinkTests COMMAND event_sink_tests)

