add_library(replay_utils STATIC utils.cpp)
target_include_directories(replay_utils PUBLIC "${PROJECT_SOURCE_DIR}/include/utils")
