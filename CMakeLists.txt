cmake_minimum_required(VERSION 3.31.5)
project(replay-recorder)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

enable_testing()

# Global include dir
include_directories("${CMAKE_SOURCE_DIR}/include")

add_subdirectory(src/recorder)
add_subdirectory(src/encoder)
add_subdirectory(src/tests)


