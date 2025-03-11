# Interface lib so we can propagate include dirs to dependents
add_library(replay_encoder_options INTERFACE)

target_include_directories(replay_encoder_options
    INTERFACE "${PROJECT_SOURCE_DIR}/include/encoder/"
)

add_executable(replay_encoder
    main.cpp
    rle.cpp
)

target_link_libraries(replay_encoder PRIVATE project_options replay_encoder_options)

