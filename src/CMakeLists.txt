add_executable(replay_recorder main.cpp)

target_include_directories(replay_recorder
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include"
)