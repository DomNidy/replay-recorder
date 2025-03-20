# --------------------------------------------------------------------
# (1) CMake Version & Project Definition
# --------------------------------------------------------------------
cmake_minimum_required(VERSION 3.31.5)
project(replay-recorder LANGUAGES CXX)

# --------------------------------------------------------------------
# (2) Global Project Settings
# --------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(VCPKG_APPINSTALL_DEPS ON)

# --------------------------------------------------------------------
# (3) Vcpkg Integration & Toolchain Configuration
# --------------------------------------------------------------------
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE
      "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "Vcpkg toolchain file")
endif()
message(STATUS "Using CMake toolchain file: ${CMAKE_TOOLCHAIN_FILE}")

# --------------------------------------------------------------------
# (4) Dependencies & External Libraries
# --------------------------------------------------------------------
find_package(spdlog CONFIG REQUIRED)

# --------------------------------------------------------------------
# (5) Common Project Options (Interface Libraries)
# --------------------------------------------------------------------
# Interface library to propagate common options and dependencies
add_library(project_options INTERFACE)
target_include_directories(project_options
                           INTERFACE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(project_options INTERFACE spdlog::spdlog_header_only)

# --------------------------------------------------------------------
# (6) Utility Functions
# --------------------------------------------------------------------
# Copies runtime DLL dependencies after target builds
function(copy_runtime_dlls target)
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying runtime DLLs for ${target}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${target}> "$<TARGET_FILE_DIR:${target}>"
    COMMAND_EXPAND_LISTS)
endfunction()

# --------------------------------------------------------------------
# (7) Subdirectories & Targets
# --------------------------------------------------------------------
add_subdirectory(libs)
add_subdirectory(src/recorder)
add_subdirectory(src/encoder)

# --------------------------------------------------------------------
# (8) Testing
# --------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
