add_library(replay_utils_options INTERFACE)
target_include_directories(replay_utils_options
                           INTERFACE "${PROJECT_SOURCE_DIR}/include/utils/")

add_library(replay_utils STATIC utils.cpp)
target_link_libraries(replay_utils PRIVATE replay_utils_options)
