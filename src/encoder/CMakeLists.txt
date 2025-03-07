add_executable(replay_encoder
    main.cpp
    rle.cpp
)

target_include_directories(replay_encoder
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../include/replay-recorder/encoder/"
)
