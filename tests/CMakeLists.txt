# Find Google Test using vcpkg (CONFIG mode)
find_package(GTest CONFIG REQUIRED)

# Add new executable target(s) (for tests)
add_executable(event_sink_tests event_sink_tests.cpp)

# Create library with code that we want to test (will link this to test executable)
add_library(event_sink_lib "${CMAKE_CURRENT_SOURCE_DIR}/../src/recorder/event_sink.cpp")
target_include_directories(event_sink_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include/replay-recorder")

# Link test executable to the test code libraries and GTest 
target_link_libraries(event_sink_tests PRIVATE event_sink_lib GTest::gtest_main GTest::gmock_main)

# Add tests to CTest
add_test(NAME EventSinkTests COMMAND event_sink_tests)
