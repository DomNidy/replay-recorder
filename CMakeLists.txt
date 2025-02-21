cmake_minimum_required(VERSION 3.31.5)

project(replay-recorder)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(replay_recorder main.cpp)