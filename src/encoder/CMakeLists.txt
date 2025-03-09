add_executable(replay_encoder
    main.cpp
    rle.cpp
)

target_include_directories(replay_encoder
    PRIVATE "${PROJECT_SOURCE_DIR}/include/replay/encoder/"
)
