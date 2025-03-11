add_executable(replay_encoder
    main.cpp
    rle.cpp
)

target_include_directories(replay_encoder
    # PUBLIC "${PROJECT_SOURCE_DIR}/include/encoder"
    PRIVATE project_options
)
