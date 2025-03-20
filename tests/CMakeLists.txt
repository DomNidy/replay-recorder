# Find Google Test using vcpkg (CONFIG mode) Config mode expects the library
# (GTest) to provide a CMake config file that list necessary info for using the
# package, such as:

# * The include directories (where the header files are located)
# * Libraries (the actual GTest library files, static or dynamic)
# * CMake targets (e.g., GTest::gtest_main, GTest::gmock_main), these are
#   abstract names that represent the libraries and make linking against them
#   easier.
# * Dependencies, libraries that GTest depends on
# * Compiler flags and definitions, any specific settings needed to compile code
#   using GTest
find_package(GTest CONFIG REQUIRED)

add_executable(
  event_sink_tests event_sink_tests.cpp
                   "${PROJECT_SOURCE_DIR}/src/recorder/event_sink.cpp")
add_executable(rle_tests rle_tests.cpp
                         "${PROJECT_SOURCE_DIR}/src/encoder/rle.cpp")
add_executable(utils_tests utils_tests.cpp)

# Link test executable to the test code libraries and GTest
target_link_libraries(
  event_sink_tests PRIVATE project_options GTest::gtest_main GTest::gmock_main
                           spdlog::spdlog)
target_link_libraries(rle_tests PRIVATE project_options replay_encoder_options
                                        GTest::gtest_main GTest::gmock_main)
target_link_libraries(
  utils_tests PRIVATE project_options replay_utils_options replay_utils
                      GTest::gtest_main GTest::gmock_main)
# Add tests to CTest
add_test(NAME EventSinkTests COMMAND event_sink_tests)
add_test(NAME RLETests COMMAND rle_tests)
add_test(NAME UtilsTests COMMAND utils_tests)

copy_runtime_dlls(event_sink_tests)
